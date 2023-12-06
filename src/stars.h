#ifndef STARS_H
#define STARS_H
#define NSTARS 1024
uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};
uint8_t za, zb, zc, zx;
uint8_t rng() {
    zx++;
    za = (za ^ zc ^ zx);
    zb = (zb + za);
    zc = ((zc + (zb >> 1)) ^ za);
    return zc;
}
#endif //STARS_H
