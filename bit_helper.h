#define CHECK_BIT(var, pos) ((var >> pos) & 1)

#define SetBit(data, y)    data |= (1 << y)    /** Set Data.Y   to 1    **/
#define ClearBit(data, y)  data &= ~(1 << y)   /** Clear Data.Y to 0    **/
#define TogleBit(data, y)     (data ^=BitVal(y))     /** Togle Data.Y  value  **/
#define Togle(data)   (data =~data )         /** Togle Data value     **/
