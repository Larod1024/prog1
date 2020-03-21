#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

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
    struct Node *next;
};

//Leert den Eingabepuffer
void clean_stdin(void)
{
    int c;
    do
    {
        c = getchar();
    }
    while (c != '\n' && c != EOF);
}

//Eigene Impelemtation von strdup da die Funktion nicht im c99 Standard enthalten ist
char *strdup(const char *str)
{
    int n = strlen(str) + 1;
    char *dup = malloc(n);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}

//Fügt einer verketteten Liste ein Element am Ende hinzu
void append(struct Node **head_ref, double *new_data)
{
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));
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
//ACHTUNG: Letztes Element wird über 1 geholt und nicht 0
double *get_Nth_from_last(struct Node *head, int n)
{
    int len = 0, i;
    struct Node *temp = head;

    while (temp != NULL)
    {
        temp = temp->next;
        len++;
    }

    if (len < n)
        return NULL;

    temp = head;

    for (i = 1; i < len - n + 1; i++)
    {
        temp = temp->next;
    }

    return temp->data;
}

//Gibt Elemente einer verketteten Liste aus
void print_list(struct Node *node, int size, bool only_print_last)
{
    int count = 1;
    while (node != NULL)
    {
        for(int i = 0; i < size; i++)
        {
            if(!only_print_last)
                printf("%2d: %10.10f\t", count, node->data[i]);
            else if(node->next == NULL)
                printf("%2d: %10.10f\t", count, node->data[i]);
        }

        if(!only_print_last)
            printf("\n");

        count++;
        node = node->next;
    }
}

//Zählt die Anzahl der Reihen einer Datei. Ignoriert leere Zeilen
//End Of Line muss immer \n sein. Kann bei Notepad erstellten csv Dateien zu Problemen kommen
int count_rows(const char *filename)
{
    FILE *file;
    if((file = fopen(filename, "r")) == NULL)
    {
        printf("Datei konnte nicht geoeffnet werden");
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
char *trim_whitespace(char *str)
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

//Lädt Datei und erstellt die Matrix/Vektoren
bool load(const char *filename, Matrix *A, Vector *b, Vector *x)
{
    //Anzahl der Rows bestimmen und Größe der structs definieren
    int rows = count_rows(filename);

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
        printf("Datei konnte nicht geoeffnet werden!");
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
    int buffer = 16384; //16KB
    char line[buffer];
    while (fgets(line, buffer, file))
    {
        char* tmp = strdup(line);

        char delimiter[] = ",";
        char *ptr;

        //Initialisieren und ersten Abschnitt erstellen
        ptr = strtok(line, delimiter);

        //Wenn Zeile leer, dann weiter
        if(strcmp(tmp, "\n") == 0)
        {
            free(tmp);
            continue;
        }

        int j = 0;

        while(ptr != NULL)
        {
            char *endptr = NULL;
            if(j < A->n)
            {
                A->data[i][j] = strtod(ptr, &endptr);
            }
            else if(j < b->n + 1)
            {
                b->data[i] = strtod(ptr, &endptr);
                //Falls der x Eintrag fehlt
                x->data[i] = 0;
            }
            //Wird nur aufgerufen wenn x Eintrag vorhanden da sonst while Schleife durch Null Pointer abgebrochen wird
            else if(j < x->n + 2)
            {
                x->data[i] = strtod(ptr, &endptr);
            }

            j++;

            endptr = trim_whitespace(endptr);

            if (!(*endptr == 0))
            {
                if (!isspace((unsigned char)*endptr))
                {
                    printf("Ein Wert konnte nicht richtig eingelesen werden: %s.\n", ptr);
                    return false;
                }
            }

            //Nächsten Abschnitt erstellen
            ptr = strtok(NULL, delimiter);
        }

        free(tmp);
        i++;
    }

    fclose(file);

    return true;
}

//Gibt an ob die Fehlerschranke unterschritten wurde
bool is_smaller_error(struct Node *node, int n, double e)
{
    double error = 0.0;
    double *last = get_Nth_from_last(node, 1);
    double *next_to_last = get_Nth_from_last(node, 2);

    if(next_to_last != NULL)
    {
        for(int i = 0; i < n; i++)
        {
            error = MAX(error, fabs(last[i] - next_to_last[i]));
        }
    }
    else //Wird bei erstem Schleifendurchlauf verwendet
    {
        for(int i = 0; i < n; i++)
        {
            error = MAX(error, fabs(last[i]));
        }
    }

    if(error < e)
        return true;
    else
        return false;
}

//Berechnung durch das Gauß-Seidel Verfahren
struct Node *gauss_seidel_method(Matrix *A, Vector *b, Vector *x, double e)
{
    //Verkette Liste die die einzelnen Iterationsschritte enthält
    struct Node *head = NULL;
    //Vektor zum speichern von Zwischenergebnissen
    double *dx = (double *) calloc(A->n, sizeof(double));
    int iterations = 0;

    for(iterations = 0; iterations < 100; iterations++)
    {
        //Berechnung des Iterationsvektors
        for(int i = 0; i < A->n; i++)
        {
            dx[i] = b->data[i];
            for(int j = 0; j < A->n; j++)
            {
                dx[i] -= A->data[i][j] * x->data[j];
            }
            dx[i] /= A->data[i][i];
            x->data[i] += dx[i];
            if(isnan(x->data[i]))
            {
                printf("Bei der Berechnung wurde ein Wert ungeueltig (NaN).\n");
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

        if(is_smaller_error(head, x->n, e))
            break;
    }

    free(dx);
    return head;
}

//Berechnung durch das Jacobi Verfahren
struct Node *jacobi_method(Matrix *A, Vector *b, Vector *x, double e)
{
    //Verkette Liste die die einzelnen Iterationsschritte enthält
    struct Node *head = NULL;
    //Vektoren zum speichern von Zwischenergebnissen
    double *dx = (double *) calloc(x->n, sizeof(double));
    double *y = (double *) calloc(x->n, sizeof(double));
    int iterations = 0;

    for(iterations = 0; iterations < 100; iterations++)
    {
        for(int i = 0; i < x->n; i++)
        {
            dx[i] = b->data[i];
            for(int j = 0; j < x->n; j++)
            {
                dx[i] -= A->data[i][j] * x->data[j];
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

        if(is_smaller_error(head, x->n, e))
            break;
    }

    free(dx);
    free(y);

    return head;
}


struct Node *solve(Method method, Matrix *A, Vector *b, Vector *x, double e)
{
    struct Node *solution = NULL;

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
        printf("Fehler! Keine gueltige Methode angegeben.\n");
    }

    return solution;
}

//Prüft String ob Applikation verlassen werden soll
bool exit_application(char *s)
{
    return strcmp(s, "EXIT") == 0 ? true : false;
}

//Prüft ob String eine Zahl ist
int is_numeric (char *s)
{
    if (s == NULL || *s == '\0' || isspace(*s))
    {
        return 0;
    }
    char *p;
    strtod (s, &p);
    return *p == '\0';
}

//Überprüft ob String Ja oder Nein ist. (Für Entscheidungen des Users)
int is_yes_no(char *s)
{
    int i = 0;
    while (s[i])
    {
        s[i] = toupper(s[i]);
        i++;
    }

    if(strcmp(s, "JA") == 0)
    {
        return 1;
    }
    else if(strcmp(s, "NEIN") == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int main()
{
    Matrix A;
    Vector b;
    Vector x;

    bool success = false;
    struct Node *solution = NULL;

    printf("Programmieren 1 - Loesen von linearen Gleichungssystemen durch Gauss-Seidel oder Jakobi.\n");
    printf("Applikation kann durch Eingabe von \"EXIT\" beendet werden.\n\n");

    do
    {
        //Eingabe Filename
        char filename[255];
        printf("Bitte den Namen der CSV Datei eingeben\n");
        bool filename_valid = false;
        do
        {
            scanf("%s", filename);

            if(exit_application(filename))
                return 0;

            //Prüft ob Datei und Leseberechtigung vorhanden
            if(access(filename, F_OK|R_OK) != -1)
            {
                filename_valid = true;
            }
            else
            {
                printf("Datei wurde nicht gefunden oder kann nicht gelesen werden. Bitte andere Datei angeben.\n");
            }

            clean_stdin();

        }
        while(!filename_valid);

        printf("\n");

        //Eingabe Verfahren
        Method method;
        printf("Bitte Verfahren zur Berechnung angeben.\n");
        printf("0 = Jakobi Verfahren, 1 = Gauss-Seidel Verfahren\n");
        bool method_valid = false;
        do
        {
            char input[255];

            scanf("%s", input);

            if(exit_application(input))
                return 0;

            int input_number = atoi(input);

            if((input_number == 0 || input_number == 1) && is_numeric(input) != 0)
            {
                method = input_number;
                method_valid = true;
            }
            else
            {
                printf("Eingabe wurde nicht erkannt. Bitte je nach Verfahren 0 oder 1 eingeben.\n");
            }

            clean_stdin();

        }
        while (!method_valid);

        printf("\n");

        //Eingabe Fehlerschranke
        double error;
        printf("Bitte eine Fehlerschranke eingeben.\n");
        bool error_valid = false;
        do
        {
            char input[255];
            scanf("%s", input);

            if(exit_application(input))
                return 0;

            if(is_numeric(input) != 0)
            {
                error = atof(input);
                error_valid = true;
            }
            else
            {
                printf("Eingegebene Fehlerschranke wurde nicht erkannt.\n");
            }

            clean_stdin();

        }
        while(!error_valid);

        printf("\n");

        //Lädt CSV Datei
        bool load_success = load(filename, &A, &b, &x);

        if (load_success)
        {
            printf("CSV-Datei erfolgreich geladen.\n");

            //Berechnungen werden durchgeführt und als Linked List zurückgegeben
            solution = solve(method, &A, &b, &x, error);

            //Ausgeben der Werte
            if(solution != NULL)
            {
                printf("Berechnung erfolgreich.\n\nGesamte Loesungsvektoren ausgeben (Ja/Nein)? Bei Nein wird nur der letzte Vektor ausgegeben.\n");

                bool output_valid = false;

                do
                {
                    char input[255];
                    scanf("%s", input);

                    if(exit_application(input))
                        return 0;


                    switch(is_yes_no(input))
                    {
                    case 1:
                        print_list(solution, x.n, false);
                        output_valid = true;
                        break;
                    case 0:
                        print_list(solution, x.n, true);
                        output_valid = true;
                        printf("\n");
                        break;
                    default:
                        printf("Eingabe wurde nicht erkannt.\n");
                    }

                    clean_stdin();

                }
                while(!output_valid);
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
        if(load_success && solution != NULL)
        {
            success = true;
        }
        else
        {
            printf("Fehler beim Laden oder Berechnen.\n");
            printf("Neue Eingabe? (Ja/Nein)\n");

            bool output_valid = false;

            do
            {
                char input[255];
                scanf("%s", input);

                if(exit_application(input))
                    return 0;

                switch(is_yes_no(input))
                {
                case 1:
                    printf("\n");
                    output_valid = true;
                    break;
                case 0:
                    success = true;
                    output_valid = true;
                    printf("\n");
                    break;
                default:
                    printf("Eingabe wurde nicht erkannt.\n");
                }

                clean_stdin();

            }
            while(!output_valid);
        }

    }
    while(!success);

    return 0;
}
