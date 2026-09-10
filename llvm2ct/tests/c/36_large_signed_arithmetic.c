// EXPECT: 0
int main()
{
    int a = 0x12345678;
    int b = 0x00123456;
    int sum = a + b;
    int difference = sum - b;
    return difference ^ a;
}
