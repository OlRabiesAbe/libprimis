#ifndef AO_H_
#define AO_H_

extern int aow, aoh;
extern GLuint aofbo[4], aotex[4], aonoisetex;
extern float aomin, aosunmin;
extern int ao, aosun, aopackdepth, aoreduce, aoreducedepth;
extern int aobilateral, aobilateralupscale;
extern int debugao;

extern void initao();
extern void cleanupao();
extern void viewao();
extern void setupao(int w, int h);

#endif
