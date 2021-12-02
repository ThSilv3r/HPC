#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))

long TimeSteps = 50;
int countLifingsPeriodics(double* currentfield, int x, int y, int w, int h);

void writeVTK2(long timestep, double *data, char prefix[1024], int localWidth, int localHeight, int threadNumber, int originX, int originY, int totalWidth)
{
    char filename[2048];
    int x, y;
    int w = localWidth - originX, h = localHeight - originY;
    int offsetX = originX;
    int offsetY = originY;
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);
    //printf("NXY:%ld\n", nxy);
    snprintf(filename, sizeof(filename), "%s_%d-%05ld%s", prefix, threadNumber, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX,
            offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *)&nxy, sizeof(long), 1, fp);

    // printf("\nThread: %d writing...\noriginX/originY:%d/%d localWidth/localHeight: %d/%d\n", threadNumber, originX, originY, w, h);

    for (y = originY; y < localHeight; y++)
    {
        for (x = originX; x < localWidth; x++)
        {
            float value = data[calcIndex(totalWidth, x, y)];
            // if (threadNumber == 0)
            //     printf("Entry: i: %d, %f | thread:%d\n", calcIndex(totalWidth, x, y), value, threadNumber);
            fwrite((unsigned char *)&value, sizeof(float), 1, fp);
        }
    }
    //printf("\nThread: %d finished\n", threadNumber);

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}


void show(double* currentfield, int w, int h) {
  printf("\033[H");
  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    //printf("\033[E");
    printf("\n");
  }
  fflush(stdout);
}


void evolve(double* currentfield, double* newfield, int w, int h, int threadNum, int Nx, int Ny, int Px, int Py, int timeStep) {
  int x,y;
  int localPx, localPy, localW, localH;
  localPx = threadNum % Px;
  localPy = threadNum % Py;
  x = Nx * localPx;
  y = Ny * localPy;
  int originX = x, originY = y;

  if(localPx == 0){
      localW = Nx;
  }else{
      localW = Nx * (localPx + 1);
  }
  if(localPy == 0){
      localH = Ny;
  }else{
      localH = Ny * (localPy +1);
  }

    while (y < localH) {

      int neighbours = countLifingsPeriodics(currentfield, x, y, w, h);
      if (currentfield[calcIndex(w, x, y)]) neighbours--;

      if(currentfield[calcIndex(w, x, y)] == 0){
          if(neighbours == 3){
              newfield[calcIndex(w, x, y)] = 1;
          }else{
              newfield[calcIndex(w, x, y)] = 0;
          }
      }
      if(currentfield[calcIndex(w, x, y)] == 1){
          if(neighbours < 2 || neighbours > 3){
              newfield[calcIndex(w, x, y)] = 0;
          }else{
              newfield[calcIndex(w, x, y)] = 1;
          }
      }
        if(x < (localW - 1)){
            x ++;
        }else{
            x = originX;
            y++;
        }
  }
  //writeVTK2(timeStep, currentfield, "gol", localW, localH, threadNum, originX, originY, w);
}

int countLifingsPeriodics(double* currentfield, int x, int y, int w, int h){
    int n = 0;
    for (int i = y-1; i <= y+1; i++){
        for (int j = x-1; j <= x+1; j++){
            if(currentfield[calcIndex(w, (j+w) % w, (i + h) % h)]){
                n++;
            }
        }
    }
    return n;
}

void filling(double *currentfield, int w, int h)
{
    int i;

    for (i = 0; i < h * w; i++)
    {
        currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
    }
}

void game(int Nx, int Ny, int Px, int Py) {
  int w = Nx * Px;
  int h = Ny * Py;

  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));

  //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));

  filling(currentfield, w, h);

    int threads;
    threads = Px * Py;

    long t;
    for (t=0;t<TimeSteps;t++) {
        //show(currentfield, w, h);

        #pragma omp parallel num_threads(threads)
        {
            int threadNum = omp_get_thread_num();
            evolve(currentfield, newfield, w, h, threadNum, Nx, Ny, Px, Py, t);
        }

        //printf("thrads: %d \n", threads);
        //printf("%ld timestep\n",t);
        //writeVTK2(t,currentfield,"gol", w, h);

        //usleep(10000);

        //SWAP
        double *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

  free(currentfield);
  free(newfield);

}

int main(int c, char **v) {
  int Nx = 0, Ny = 0, Px = 0, Py = 0;
  if (c > 1) Nx = atoi(v[1]); ///< read width
  if (c > 2) Ny = atoi(v[2]); ///< read height
  if (c > 3) Px = atoi(v[3]); ///< read number of Px
  if (c >4) Py = atoi(v[4]); ///< read numbers of Py
  if (Nx <= 0) Nx = 4096/1; ///< default width
  if (Ny <= 0) Ny = 4096/8; ///< default height
  if (Px <= 0) Px = 1; ///< default Px
  if (Py <= 0) Py = 8; ///< default Py
  TimeSteps = 54;

  game(Nx, Ny, Px, Py);
}
