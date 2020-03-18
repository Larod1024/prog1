#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

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

struct Node
{
    double *data;
    struct Node* next;
};

void clean_stdin(void)
{
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

//F�gt einer verketteten Liste ein Element am Ende hinzu
void append(struct Node** head_ref, double* new_data)
{
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
    struct Node *last = *head_ref;

    new_node->data  = new_data;

    new_node->next = NULL;

    if (*head_ref == NULL)
    {
        *head_ref = new_node;
        return;
    }

    while (last->next != NULL)
    {
        last = last->next;
    }

    last->next = new_node;
    return;
}

//Holt das nte Elemente einer verketteten Liste von hinten
//ACHTUNG: Letztes Element wird �ber 1 geholt und nicht 0
double* getNthFromLast(struct Node* head, int n)
{
    int len = 0, i;
    struct Node* temp = head;

    while (temp != NULL)
    {
        temp = temp->next;
        len++;
    }

    if (len < n)
    {
        return NULL;
    }

    temp = head;

    for (i = 1; i < len - n + 1; i++)
    {
        temp = temp->next;
    }

    return temp->data;
}

//Gibt Elemente einer verketteten Liste aus
void printList(struct Node *node, int size, bool onlyPrintLast)
{
    int count = 1;
    while (node != NULL)
    {
        for(int i = 0; i < size; i++)
        {
            if(!onlyPrintLast)
            {
                printf("%2d: %10.10f\t", count, node->data[i]);
            }
            else if(node->next == NULL)
            {
                printf("%2d: %10.10f\t", count, node->data[i]);
            }
        }

        if(!onlyPrintLast)
            printf("\n");

        count++;
        node = node->next;
    }
}

//Z�hlt die Anzahl der Reihen einer Datei. Ignoriert leere Zeilen
//End Of Line muss immer \n sein. Kann bei Notepad erstellten csv Dateien zu Problemen kommen
int countRows(const char *filename)
{
    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Datei konnte nicht ge�ffnet werden");
        return 0;
    };

    int chbefore=0;
    int ch=0;
    int rows=0;

    rows++;
    while(!feof(file))
    {
        chbefore = ch;
        ch = fgetc(file);
        if(ch == '\n' && chbefore != 10)
        {
            rows++;
        }
    }

    // von der Anzahl der Rows muss immer 1 abgezogen werden
    // siehe --> https://stackoverflow.com/questions/5431941/why-is-while-feof-file-always-wrong
    return rows - 1;
}

//Entfernt Leerzeichen am Anfang und Ende eines Strings
char *trimwhitespace(char *str)
{
    char *end;

    while(isspace((unsigned char)*str))
    {
        str++;
    }

    //Wenn string nur aus Leerzeichen besteht
    if(*str == 0)
    {
        return str;
    }

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end))
    {
        end--;
    }

    //Neuen Terminator ans Ende
    end[1] = '\0';

    return str;
}

//L�dt Datei und erstellt die Matrix/Vektoren
bool load(const char *filename, Matrix *A, Vector *b, Vector *x)
{
    //Anzahl der Rows bestimmen und Gr��e der structs definieren
    int rows = countRows(filename);

    if(rows == 0)
    {
        printf("Datei ist leer!");
        return false;
    }

    A->n = rows;
    b->n = rows;
    x->n = rows;

    //Datei laden
    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Datei konnte nicht ge�ffnet werden!");
        return false;
    };

    //Matrix/Vektoren im Speicher allokieren
    A->data = (double **)calloc(A->n, sizeof(double *));
    for (int i = 0; i < A->n; ++i)
    {
        A->data[i] = (double *)calloc(A->n, sizeof(double));
    }
    b->data = (double *)calloc(b->n, sizeof(double));
    x->data = (double *)calloc(x->n, sizeof(double));

    //Datei lesen
    int i = 0;
    int buffer = 8192; //8KB
    char line[buffer];
    while (fgets(line, buffer, file))
    {
        char* tmp = strdup(line);

        char delimiter[] = ",";
        char *ptr;

        //Initialisieren und ersten Abschnitt erstellen
        ptr = strtok(line, delimiter);

        //Wenn Zeile leer dann weiter
        if(strcmp(tmp, "\n") == 0)
        {
            free(tmp);
            continue;
        }

        int j = 0;

        while(ptr != NULL)
        {
            if(j < A->n)
            {
                A->data[i][j] = atof(trimwhitespace(ptr));
            }
            else if(j < b->n + 1)
            {
                b->data[i] = atof(trimwhitespace(ptr));
                //Falls der x Eintrag fehlt
                x->data[i] = 0;
            }
            //Wird nur aufgerufen wenn x Eintrag vorhanden da sonst while Schleife durch Null Pointer abgebrochen wird
            else if(j < x->n + 2)
            {
                x->data[i] = atof(trimwhitespace(ptr));
            }

            j++;

            //N�chsten Abschnitt erstellen
            ptr = strtok(NULL, delimiter);
        }

        free(tmp);
        i++;
    }

    fclose(file);

    return true;
}

//Berechnung durch das Gau�-Seidel Verfahren
struct Node* gauss_seidel_method(Matrix *A, Vector *b, Vector *x, double e)
{
    //Verkette Liste mit R�ckgabewert
    struct Node* head = NULL;
    //Vektor zum berechnen von Zwischenergebnisen
    double *dx = (double*) calloc(A->n,sizeof(double));
    //Z�hlvariable f�r Durchl�ufe
    int iterations = 0;

    double error = 0.0;

    for(iterations = 0; iterations < 100; iterations++)
    {
        error = 0.0;
        //Berechnung des Iterationsvektors
        for(int i = 0; i < A->n; i++)
        {
            dx[i] = b->data[i];
            for(int j = 0; j < A->n; j++)
            {
                dx[i] -= A->data[i][j]*x->data[j];
            }
            dx[i] /= A->data[i][i];
            x->data[i] += dx[i];
            if(isnan(x->data[i]))
            {
                printf("Bei der Berechnung ist ein Wert NaN.\n");
                return NULL;
            }
        }

        //Speichert Vektor in die verkettete Liste
        double *tmp;
        tmp = (double *)calloc(b->n, sizeof(double));
        for(int i = 0; i < x->n; i++)
        {
            tmp[i] = x->data[i];
        }
        append(&head, tmp);

        //Rechnet den Fehler aus dem vorgegangenen Vektor
        if(iterations > 0)
        {
            double* last = getNthFromLast(head, 1);
            double* nexttolast = getNthFromLast(head, 2);
            for(int i = 0; i < x->n; i++)
            {
                error = MAX(error, fabs(last[i] - nexttolast[i]));
            }
        }
        else //Wird bei erstem Schleifendurchlauf verwendet
        {
            for(int i = 0; i < x->n; i++)
            {
                error = MAX(error, fabs(x->data[i]));
            }
        }

        //Wenn Fehler kleiner Fehlerschranke wird Algo beendet
        if(error < e)
        {
            break;
        }
    }

    free(dx);
    return head;
}


struct Node* jacobi_method(Matrix *A, Vector *b, Vector *x, double e)
{
    struct Node* head = NULL;
    double *dx = (double*) calloc(x->n, sizeof(double));
    double *y = (double*) calloc(x->n, sizeof(double));
    int iterations = 0;
    double error = 0.0;

    for(iterations = 0; iterations < 100; iterations++)
    {
        error = 0;
        for(int i = 0; i < x->n; i++)
        {
            dx[i] = b->data[i];
            for(int j = 0; j < x->n; j++)
            {
                dx[i] -= A->data[i][j]*x->data[j];
            }
            dx[i] /= A->data[i][i];
            y[i] += dx[i];
        }

        for(int i = 0; i < x->n; i++)
        {
            x->data[i] = y[i];
            if(isnan(y[i]))
            {
                printf("Bei der Berechnung ist ein Wert NaN.\n");
                return NULL;
            }

        }

        //Speichert Vektor in die verkettete Liste
        double *tmp;
        tmp = (double *)calloc(x->n, sizeof(double));
        for(int i = 0; i < x->n; i++)
        {
            tmp[i] = x->data[i];
        }
        append(&head, tmp);

        //Rechnet den Fehler aus dem vorgegangenen Vektor
        if(iterations > 0)
        {
            double* last = getNthFromLast(head, 1);
            double* nexttolast = getNthFromLast(head, 2);
            for(int i = 0; i < x->n; i++)
            {
                error = MAX(error, fabs(last[i] - nexttolast[i]));
            }
        }
        else //Wird bei erstem Schleifendurchlauf verwendet
        {
            for(int i = 0; i < x->n; i++)
            {
                error = MAX(error, fabs(x->data[i]));
            }
        }

        //Wenn Fehler kleiner Fehlerschranke wird Algo beendet
        if(error < e)
        {
            break;
        }

    }

    free(dx);
    free(y);

    return head;
}


struct Node* solve(Method method, Matrix *A, Vector *b, Vector *x, double e)
{
    struct Node* solution = NULL;

    if(method == JACOBI)
    {
        solution = jacobi_method(A, b, x, e);
    }
    else if(method == GAUSS_SEIDEL)
    {
        solution = gauss_seidel_method(A, b, x, e);
    }
    else
    {
        printf("Fehler! Keine gueltige Methode angegeben");
    }

    return solution;
}

bool exitApplication(char* input)
{
    return strcmp(input, "EXIT") == 0 ? true : false;
}

int isNumeric (char* s)
{
    if (s == NULL || *s == '\0' || isspace(*s))
    {
        return 0;
    }
    char * p;
    strtod (s, &p);
    return *p == '\0';
}

int main()
{
    Matrix A;
    Vector b;
    Vector x;

    bool success = false;
    struct Node* solution = NULL;

    printf("Programmieren 1 - Loesen von linearen Gleichungssystemen durch Gauss-Seidel oder Jakobi.\n");
    printf("Applikation kann durch Eingabe von \"EXIT\" beendet werden.\n\n");

    do
    {


        //Eingabe Filename
        char filename[255];
        printf("Bitte den Namen der CSV Datei eingeben\n");
        bool filenameValid = false;
        do
        {
            scanf("%s", filename);

            if(exitApplication(filename))
                return 0;

            if(access(filename, F_OK|R_OK) != -1)
            {
                filenameValid = true;
            }
            else
            {
                printf("Datei wurde nicht gefunden oder kann nicht gelesen werden. Bitte andere Datei angeben.\n");
            }

            clean_stdin();

        }
        while(!filenameValid);

        printf("\n");

        //Eingabe Verfahren
        Method method;
        printf("Bitte Verfahren zur Berechnung angeben.\n");
        printf("0 = Jakobi Verfahren, 1 = Gauss-Seidel Verfahren\n");
        bool methodValid = false;
        do
        {
            char input[255];

            scanf("%s", input);

            if(exitApplication(input))
                return 0;


            int inputNumber = atoi(input);

            if((inputNumber == 0 || inputNumber == 1) && isNumeric(input) != 0)
            {
                method = inputNumber;
                methodValid = true;
            }
            else
            {
                printf("Eingabe wurde nicht erkannt. Bitte je nach Verfahren 0 oder 1 eingeben.\n");
            }

            clean_stdin();

        }
        while (!methodValid);

        printf("\n");

        //Eingabe Fehlerschranke
        double error;
        printf("Bitte eine Fehlerschranke eingeben.\n");
        bool errorValid = false;
        do
        {
            char input[255];
            scanf("%s", input);

            if(exitApplication(input))
                return 0;

            if(isNumeric(input) != 0)
            {
                error = atof(input);
                errorValid = true;
            }
            else
            {
                printf("Eingegebene Fehlerschranke wurde nicht erkannt.\n");
            }

            clean_stdin();

        }
        while(!errorValid);

        printf("\n");

        //L�dt CSV Datei
        bool loadSuccess = load(filename, &A, &b, &x);

        if (loadSuccess)
        {
            printf("CSV-Datei erfolgreich geladen.\n");

            //Berechnungen werden durchgef�hrt und als Linked List zur�ckgegeben
            solution = solve(method, &A, &b, &x, error);

            //Ausgeben der Werte
            if(solution != NULL)
            {
                printf("Berechnung erfolgreich.\n\nGesamte Loesungsvektoren ausgeben (Ja/Nein)? Bei Nein wird nur der letzte Vektor ausgegeben.\n");

                bool outputValid = false;

                do
                {
                    char input[255];
                    scanf("%s", input);

                    if(exitApplication(input))
                        return 0;

                    int i = 0;

                    while (input[i])
                    {
                        input[i] = toupper(input[i]);
                        i++;
                    }

                    if(strcmp(input, "JA") == 0)
                    {
                        printList(solution, x.n, false);
                        outputValid = true;
                    }
                    else if(strcmp(input, "NEIN") == 0)
                    {
                        printList(solution, x.n, true);
                        outputValid = true;
                        printf("\n");
                    }
                    else
                    {
                        printf("Eingabe wurde nicht erkannt.\n");
                    }

                    clean_stdin();
                }
                while(!outputValid);

            }
            else
            {
                printf("Fehler beim Berechnen der Loesung.\n");
            }
        }
        else
        {
            printf("Fehler beim Laden der CSV-Datei.\n");
        }

        printf("\n");

        //Abfrage ob bei Fehler die Eingabe wiederholt werden soll
        if(loadSuccess && solution != NULL)
        {
            success = true;
        }
        else
        {
            printf("Fehler beim Laden oder Berechnen.\n");
            printf("Neue Eingabe (Ja/Nein)?\n");

            bool outputValid = false;

            do
            {
                char input[255];
                scanf("%s", input);

                if(exitApplication(input))
                    return 0;

                int i = 0;

                while (input[i])
                {
                    input[i] = toupper(input[i]);
                    i++;
                }

                if(strcmp(input, "JA") == 0)
                {
                    printf("\n");
                    outputValid = true;
                }
                else if(strcmp(input, "NEIN") == 0)
                {
                    success = true;
                    outputValid = true;
                    printf("\n");
                }
                else
                {
                    printf("Eingabe wurde nicht erkannt.\n");
                }

                clean_stdin();

            }
            while(!outputValid);
        }

    }
    while(!success);

    return 0;
}
