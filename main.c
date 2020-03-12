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


bool load(const char *filename, Matrix *A, Vector *b, Vector *x)
{
    int rows = countRows(filename);

    A->n = rows;
    b->n = rows;
    x->n = rows;

    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Error opening file!");
        return false;
    };


    A->data = (double **)malloc(A->n * sizeof(double *));
    for (int i = 0; i < A->n; ++i)
    {
        A->data[i] = (double *)malloc(A->n * sizeof(double));
    }

    b->data = (double *)malloc(b->n * sizeof(double));
    x->data = (double *)malloc(x->n * sizeof(double));

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

            if(j < A->n)
            {
                A->data[i][j] = atof(ptr);

            }
            else
            {
                b->data[i] = atof(ptr);
                x->data[i] = 0;

            }

            j++;

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
/*
void gauss_seidel(Matrix *A, Vector *b, Vector *x, double e)
{
    int count, t, limit;
    float temp, error, a, sum = 0;
    float allowed_error;

    limit = A->n;
    allowed_error = 0.1;


    for(count = 1; count <= limit; count++)
    {
        b->data[count] = 0;
    }
    do
    {
        a = 0;
        for(count = 1; count <= limit; count++)
        {
            sum = 0;
            for(t = 1; t;  a)
            {
                a = error;
            }
            b->data[count] = temp;
            printf("\nY[%d]=\t%f", count, b->data[count]);
        }
        printf("\n");
    }
    while(a >= allowed_error);

    printf("\n\nSolution\n\n");

    for(count = 1; count <= limit; count++)
    {
        printf(" \nY[%d]:\t%f", count, b->data[count]);
    }
    return;
}
*/
void run_gauss_seidel_method(Matrix *A, Vector *b, Vector *x, double epsilon, int maxit, int *numit)
{
    double *dx = (double*) calloc(A->n ,sizeof(double));
    int i, j, k;

    for(k = 0; k < maxit; k++)
    {
        double sum = 0.0;
        for(i = 0; i < A->n; i++)
        {
            dx[i] = b->data[i];
            for(j = 0; j < A->n; j++)
            {
                dx[i] -= A->data[i][j]*x->data[j];
            }
            dx[i] /= A->data[i][i];
            x->data[i] += dx[i];
            sum += ( (dx[i] >= 0.0) ? dx[i] : -dx[i]);
            printf("%d", x->data[i]);
        }
        printf("%4d : %.3e\n",k,sum);
        if(sum <= epsilon)
            break;
    }
    *numit = k+1;
    free(dx);
}


void solve(Method method, Matrix *A, Vector *b, Vector *x, double e)
{
    int count = 0;
    run_gauss_seidel_method(A, b, x, 1.0e-10, 2*A->n*A->n, &count);
    for(int i = 0; i < A->n; i++)
    {
        printf("%20.10f\t", x->data[i]);
    }
    return;
}



int main()
{
    Matrix A;
    Vector b;
    Vector x;

    //char filename[255];
    //printf("Please input the name of the CSV file\n");
    //scanf("%s", filename);



    // Zum Testen um nicht jedes mal neu die Datei einzugeben.
    char filename[] = { "konv3.csv" };
    if (!load(filename, &A, &b, &x))
    {
        printf("Error loading");
    };

    solve(0, &A, &b, &x, 0);

    return 0;
}
