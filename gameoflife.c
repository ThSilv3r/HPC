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
    if (isDirectoryExists("vti") == 0)
        mkdir("vti", 0777);
    int offsetX = originX;
    int offsetY = originY;
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);
    snprintf(filename, sizeof(filename), "vti/%s_%d-%05ld%s", prefix, threadNumber, timestep, ".vti");
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

    for (y = originY; y < localHeight; y++)
    {
        for (x = originX; x < localWidth; x++)
        {
            float value = data[calcIndex(totalWidth, x, y)];
            fwrite((unsigned char *)&value, sizeof(float), 1, fp);
        }
    }

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

void filling(double *currentfield, int w, int h, char inputConfiguration[])
{
    //int i;

    readInputConfig(currentfield, w, h, inputConfiguration);

    /*for (i = 0; i < h * w; i++)
    {
        currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
    }*/
}


void game(int Nx, int Ny, int Px, int Py) {
  int w = Nx * Px;
  int h = Ny * Py;

  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));

  //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));
  char inputConfiguration[] = "#N $rats\n#O David Buckingham\n#C A period 6 oscillator found in 1972.\n#C www.conwaylife.com/wiki/index.php?title=$rats\nx = 12, y = 11, rule = B3/S23\n5b2o5b$6bo5b$4bo7b$2obob4o3b$2obo5bobo$3bo2b3ob2o$3bo4bo3b$4b3obo3b$7bo4b$6bo5b$6b2o!";

  filling(currentfield, w, h, inputConfiguration);


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

int main(int argc, char *argv[])
{
    int Nx = 0;
    int Ny = 0;
    int Px = 0;
    int Py = 0;
    char fileName[1024] = "";

    if (argc > 1)
        TimeSteps = atoi(argv[1]);
    if (argc > 2)
        Nx = atoi(argv[2]);
    if (argc > 3)
        Ny = atoi(argv[3]);
    if (argc > 4)
        Px = atoi(argv[4]);
    if (argc > 5)
        Py = atoi(argv[5]);
    if (Nx <= 0)
        Nx = 200;
    if (Ny <= 0)
        Ny = 200;
    if (Px <= 0)
        Px = 2;
    if (Py <= 0)
        Py = 4;
    if (TimeSteps <= 0)
        TimeSteps = 100;

    int w = Nx * Px;
    int h = Ny * Py;

    int bufferSize = w * h;

    char *readBuffer = calloc(bufferSize, sizeof(char));

    if (argc > 6)
    {
        snprintf(fileName, sizeof(fileName), "%s", argv[6]);

        printf("Filename: %s\n", fileName);
        FILE *fp;
        fp = fopen(fileName, "r");
        if (!fp)
        {
            printf("Could not open File.\n");
            return 1;
        }
        fread(readBuffer, sizeof(char), bufferSize, fp);
        fclose(fp);
    }
    // default:
    game(Nx, Ny, Px, Py, readBuffer);
}

void readInputConfig(double *currentfield, int width, int height, char inputConfiguration[])
{
    int i = 0;
    int x = 0;
    int y = height - 1;

    char currentCharacter;
    char nextCharacter;
    int currentDigit;
    bool isComment = true;

    while (isComment)
    {
        if (inputConfiguration[i] == '#')
        {
            isComment = false;
            while (inputConfiguration[i] != '\n')
                i++;
            isComment = inputConfiguration[++i] == '#';
        }
    }

    int buffIndex = 0;
    char bufferX[10];
    if (inputConfiguration[i] == 'x' || inputConfiguration[i] == 'X')
    {
        while (isdigit(inputConfiguration[i]) == false)
            i++;
        while (isdigit(inputConfiguration[i]))
            bufferX[buffIndex++] = inputConfiguration[i++];
    }
    int xMax = atoi(bufferX);

    while (inputConfiguration[i] != 'y' && inputConfiguration[i] != 'Y')
        i++;

    char bufferY[10];
    buffIndex = 0;
    if (inputConfiguration[i] == 'y' || inputConfiguration[i] == 'Y')
    {
        while (isdigit(inputConfiguration[i]) == false)
            i++;
        while (isdigit(inputConfiguration[i]))
            bufferY[buffIndex++] = inputConfiguration[i++];
        while (inputConfiguration[i] != '\n')
            i++;
    }

    int yMax = atoi(bufferY);

    // printf("Max x/y: %d/%d\n", xMax, yMax);

    while (inputConfiguration[i] != '!')
    {
        currentCharacter = inputConfiguration[i];
        if (currentCharacter == '$')
        {
            x = 0;
            y--;
        }
        else if (currentCharacter == 'b')
            x++;
        else if (currentCharacter == 'o')
            currentfield[calcIndex(width, x, y)] = 1;
        else if (isdigit(currentCharacter))
        {
            currentDigit = currentCharacter - '0';
            // printf("The Char: %c is digit: %d\n", currentCharacter, currentDigit);
            nextCharacter = inputConfiguration[++i];
            if (nextCharacter == 'b')
                x += currentDigit;
            else if (nextCharacter == 'o')
            {
                int upperBound = x + currentDigit;
                for (; x < upperBound; x++)
                    currentfield[calcIndex(width, x, y)] = 1;
            }
        }
        i++;
    } // printf("The String is: %s | The array size is: %d\n", inputConfiguration, stringLength);
}