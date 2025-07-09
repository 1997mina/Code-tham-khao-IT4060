#include <stdio.h>

struct student
{
    char name[20];
    int eng;
    int math;
    int phys;
    double mean;
};

static struct student data[] =
{
    {"Tuan", 82, 72, 58, 0.0}, 
    {"Nam", 77, 82, 79, 0.0},
    {"Khanh", 52, 62, 39, 0.0}, 
    {"Phuong", 61, 82, 88, 0.0}
};


char grade (double mean)
{
    if (mean < 60)
        return 'D';
    else if (mean < 70) 
        return 'C';
    else if (mean < 80) 
        return 'B';
    else if (mean < 90)
        return 'A';
    else
        return 'S';
}

int main ()
{
    for (int i = 0; i < 4; ++i)
    {
        data[i].mean = (data[i].eng + data[i].math + data[i].phys) / 3.0; 
        struct student s = data[i];

        printf ("- Ten học sinh: %s\n", s.name); 
        printf (" + Diem Tieng Anh: %d\n", s.eng);
        printf (" + Diem Toan: %d\n", s.math);
        printf (" + Diem Vat Ly: %d\n", s.phys);
        printf (" -> Diem trung binh: %.2f\n", s.mean);
        printf (" -> Hang: %c\n\n", grade (s.mean));
    }
    
    return 0;
}