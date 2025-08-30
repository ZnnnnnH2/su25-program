#include <iostream>
#include <stdlib.h>

using namespace std;

// 需要填空
bool isValid(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8)
{
    // Check if any two queens are in the same row
    if (i1 == i2 || i1 == i3 || i1 == i4 || i1 == i5 || i1 == i6 || i1 == i7 || i1 == i8 ||
        i2 == i3 || i2 == i4 || i2 == i5 || i2 == i6 || i2 == i7 || i2 == i8 ||
        i3 == i4 || i3 == i5 || i3 == i6 || i3 == i7 || i3 == i8 ||
        i4 == i5 || i4 == i6 || i4 == i7 || i4 == i8 ||
        i5 == i6 || i5 == i7 || i5 == i8 ||
        i6 == i7 || i6 == i8 ||
        i7 == i8)
    {
        return false;
    }

    // Check if any two queens are on the same diagonal
    // For diagonal, the difference between row and column should be unique
    int positions[8] = {i1, i2, i3, i4, i5, i6, i7, i8};

    for (int i = 0; i < 8; i++)
    {
        for (int j = i + 1; j < 8; j++)
        {
            // Check if queens are on the same diagonal
            // Diagonal: |row1 - row2| == |col1 - col2|
            if (abs((i + 1) - (j + 1)) == abs(positions[i] - positions[j]))
            {
                return false;
            }
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    int Num = 0;
    for (int i1 = 1; i1 <= 8; i1++)
    {
        for (int i2 = 1; i2 <= 8; i2++)
        {
            for (int i3 = 1; i3 <= 8; i3++)
            {
                for (int i4 = 1; i4 <= 8; i4++)
                {
                    for (int i5 = 1; i5 <= 8; i5++)
                    {
                        for (int i6 = 1; i6 <= 8; i6++)
                        {
                            for (int i7 = 1; i7 <= 8; i7++)
                            {
                                for (int i8 = 1; i8 <= 8; i8++)
                                {
                                    if (isValid(i1, i2, i3, i4, i5, i6, i7, i8))
                                    {
                                        Num++;
                                        printf("solution %d: %d %d %d %d %d %d %d\n", Num, i1, i2, i3, i4, i5, i6, i7, i8);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
