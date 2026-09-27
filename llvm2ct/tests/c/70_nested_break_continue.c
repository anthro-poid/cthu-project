// EXPECT: 5

int main()
{
    int total = 0;

    for ( int outer = 0; outer < 2; ++ outer )
    {
        for ( int inner = 0; inner < 3; ++ inner )
        {
            if ( inner == 1 )
                continue;

            if ( outer == 1 && inner == 2 )
                break;

            total += outer * 3 + inner;
        }
    }

    return total;
}
