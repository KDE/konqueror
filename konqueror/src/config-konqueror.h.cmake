/* Use mallinfo field hblkhd */
#define KDE_MALLINFO_FIELD_hblkhd 1

/* Use mallinfo field uordblks */
#define KDE_MALLINFO_FIELD_uordblks 1

/* Use mallinfo field usmblks */
/* #undef KDE_MALLINFO_FIELD_usmblks */

/* mallinfo() is available in <malloc.h> */
#cmakedefine KDE_MALLINFO_MALLOC 1

/* mallinfo() is available in <stdlib.h> */
#cmakedefine KDE_MALLINFO_STDLIB 1

