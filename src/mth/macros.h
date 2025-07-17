#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define SBV(v, b) ((v) |= (b))
#define CBV(v, b) ((v) &= ~(b))
#define BTV(v, b) ((v) & (b))
