#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

extern char* strdup(const char*);

typedef struct
{
    int n;
    double **data;
} Matrix;

typedef struct
{
    int n;
    double *data;
} Vector;

typedef enum
{
    JACOBI = 0, GAUSS_SEIDEL = 1
} Method;


int countRows(const char *filename)
{

    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Error opening file!");
        return 0;
    };

    int ch=0;
    int rows=0;


    rows++;
    while(!feof(file))
    {
        ch = fgetc(file);
        if(ch == '\n')
        {
            rows++;
        }
    }

    printf("rows: %d\n", rows-1);
    // von der Anzahl der Rows muss immer 1 abgezogen werden
    // siehe --> https://stackoverflow.com/questions/5431941/why-is-while-feof-file-always-wrong
    return rows - 1;
}

int countCols(const char *filename)
{
    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Error opening file!");
        return 0;
    };

    int cols = 0;
    int buffer = 8192;
    char line[buffer];

    char *ptr;

    fgets(line, buffer, file);

    char delimiter[] = ",";

    // initialisieren und ersten Abschnitt erstellen
    ptr = strtok(line, delimiter);

    while(ptr != NULL)
    {
        cols++;
        ptr = strtok(NULL, delimiter);
    }

    printf("cols: %d\n", cols);

    return cols;
}


bool load(const char *filename, Matrix *A, Vector *b, Vector *x)
{
    int rows = countRows(filename);
    int cols = countCols(filename);

    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Error opening file!");
        return false;
    };




    double **data;
    data = (double **)malloc(rows * sizeof(double *));
    for (int i = 0; i < rows; ++i)
    {
        data[i] = (double *)malloc(cols * sizeof(double));
    }

    int i = 0;
    int buffer = 8192; //8KB
    char line[buffer];
    while (fgets(line, buffer, file))
    {
        // double row[ssParams->nreal + 1];
        char* tmp = strdup(line);

        char delimiter[] = ",";
        char *ptr;

        // initialisieren und ersten Abschnitt erstellen
        ptr = strtok(line, delimiter);

        int j = 0;

        while(ptr != NULL)
        {
            data[i][j] = atof(ptr);
            printf("%20.10f\t", data[i][j]);
            // naechsten Abschnitt erstellen
            ptr = strtok(NULL, delimiter);
        }

        printf("\n");

        free(tmp);
        i++;
    }

    fclose(file);

    return true;
}

void solve(Method method, Matrix *A, Vector *b, Vector *x, double e)
{
    return;
}

int main()
{
    //char filename[255];
    //printf("Please input the name of the CSV file\n");
    //scanf("%s", filename);

    // Zum Testen um nicht jedes mal neu die Datei einzugeben.
    char filename[] = { "konv500.csv" };
    if (load(filename, NULL, NULL, NULL) == false)
    {
        printf("Error loading");
    };

    return 0;
}


