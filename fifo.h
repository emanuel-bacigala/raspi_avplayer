typedef struct
{
    unsigned char *pixBuffer;
    long int pts;
} buff_element;

void fifoInit(int fifoLen, int elementLen);
buff_element* fifoPut(void);
buff_element* fifoGet(void);
inline int fifoGetNumElements(void);
void fifoRelease(void);

