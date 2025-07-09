#include <stdio.h>

typedef struct student
{
    char name[20];
    int eng;
    int math;
    int phys;
} STUDENT;

STUDENT data[] = 
{
    {"Tuan", 82, 72, 58},
    {"Nam", 77, 82, 79},
    {"Khanh", 52, 62, 39},
    {"Phuong", 61, 82, 88}
};

int main ()
{
    STUDENT *p = data;

    while (p -> name[0] != '\0')
    {
        printf ("- Ten học sinh: %s\n", p -> name); 
        printf (" + Diem Tieng Anh: %d\n", p -> eng);
        printf (" + Diem Toan: %d\n", p -> math);
        printf (" + Diem Vat Ly: %d\n\n", p -> phys);
        
        p++;
    }
    
    return 0;
}