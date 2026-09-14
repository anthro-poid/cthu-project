// EXPECT: 1

#include <stdbool.h>

bool less_after_offset( unsigned char byte, int limit )
{
    int widened = byte;
    goto offset;

offset: ;
    int adjusted = widened + 20;
    goto compare;

compare: ;
    return adjusted < limit;
}

int main()
{
    return less_after_offset( 200, 230 );
}
