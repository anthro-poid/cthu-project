// EXPECT: 4294967294
int main()
{
    int one = 1;
    int eight = 8;
    int negative = one - eight;
    return negative >> 2;
}
