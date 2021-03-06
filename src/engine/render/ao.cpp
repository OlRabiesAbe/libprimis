/* ao.cpp: screenspace ambient occlusion
 *
 * Screenspace ambient occlusion is a way to simulate darkening of corners which
 * do not recieve as much diffuse light as other areas. SSAO relies on the depth
 * buffer of the scene to determine areas which appear to be creases and
 * darkens those areas. Various settings allow for more or less artifact-free
 * rendition of this darkening effect.
 */

#include "engine.h"

#include "rendergl.h"
#include "rendertimers.h"
#include "renderwindow.h"

#include "interface/control.h"

int aow  = -1,
    aoh  = -1;
GLuint aofbo[4] = { 0, 0, 0, 0 },
       aotex[4] = { 0, 0, 0, 0 },
       aonoisetex = 0;

void cleanupao(); //forward decl needed for VAR macros
VARFP(ao, 0, 1, 1, { cleanupao(); cleardeferredlightshaders(); });
FVARR(aoradius, 0, 5, 256);
FVAR(aocutoff, 0, 2.0f, 1e3f);
FVARR(aodark, 1e-3f, 11.0f, 1e3f);
FVARR(aosharp, 1e-3f, 1, 1e3f);
FVAR(aoprefilterdepth, 0, 1, 1e3f);
FVARR(aomin, 0, 0.25f, 1);
VARFR(aosun, 0, 1, 1, cleardeferredlightshaders());
FVARR(aosunmin, 0, 0.5f, 1);
VARP(aoblur, 0, 4, 7);
VARP(aoiter, 0, 0, 4);
VARFP(aoreduce, 0, 1, 2, cleanupao());
VARF(aoreducedepth, 0, 1, 2, cleanupao());
VARFP(aofloatdepth, 0, 1, 2, initwarning("AO setup", Init_Load, Change_Shaders));
VARFP(aoprec, 0, 1, 1, cleanupao());
VAR(aodepthformat, 1, 0, 0);
VARF(aonoise, 0, 5, 8, cleanupao());
VARFP(aobilateral, 0, 3, 10, cleanupao());
FVARP(aobilateraldepth, 0, 4, 1e3f);
VARFP(aobilateralupscale, 0, 0, 1, cleanupao());
VARF(aopackdepth, 0, 1, 1, cleanupao());
VARFP(aotaps, 1, 12, 12, cleanupao());
VARF(aoderivnormal, 0, 0, 1, cleanupao());
VAR(debugao, 0, 0, 1);

static Shader *ambientobscuranceshader = nullptr;

Shader *loadambientobscuranceshader()
{
    string opts;
    int optslen = 0;

    bool linear = aoreducedepth && (aoreduce || aoreducedepth > 1);
    if(linear)
    {
        opts[optslen++] = 'l';
    }
    if(aoderivnormal)
    {
        opts[optslen++] = 'd';
    }
    if(aobilateral && aopackdepth)
    {
        opts[optslen++] = 'p';
    }
    opts[optslen] = '\0';

    DEF_FORMAT_STRING(name, "ambientobscurance%s%d", opts, aotaps);
    return generateshader(name, "ambientobscuranceshader \"%s\" %d", opts, aotaps);
}

void loadaoshaders()
{
    ambientobscuranceshader = loadambientobscuranceshader();
}

void clearaoshaders()
{
    ambientobscuranceshader = nullptr;
}

void setupao(int w, int h)
{
    int sw = w>>aoreduce,
        sh = h>>aoreduce;

    if(sw == aow && sh == aoh)
    {
        return;
    }
    aow = sw;
    aoh = sh;
    if(!aonoisetex)
    {
        glGenTextures(1, &aonoisetex);
    }
    bvec *noise = new bvec[(1<<aonoise)*(1<<aonoise)];
    for(int k = 0; k < (1<<aonoise)*(1<<aonoise); ++k)
    {
        noise[k] = bvec(vec(randomfloat(2)-1, randomfloat(2)-1, 0).normalize());
    }
    createtexture(aonoisetex, 1<<aonoise, 1<<aonoise, noise, 0, 0, GL_RGB, GL_TEXTURE_2D);
    delete[] noise;

    bool upscale = aoreduce && aobilateral && aobilateralupscale;
    GLenum format = aoprec ? GL_R8 : GL_RGBA8,
           packformat = aobilateral && aopackdepth ? (aodepthformat ? GL_RG16F : GL_RGBA8) : format;
    int packfilter = upscale && aopackdepth && !aodepthformat ? 0 : 1;
    for(int i = 0; i < (upscale ? 3 : 2); ++i)
    {
        if(!aotex[i])
        {
            glGenTextures(1, &aotex[i]);
        }
        if(!aofbo[i])
        {
            glGenFramebuffers_(1, &aofbo[i]);
        }
        createtexture(aotex[i], upscale && i ? w : aow, upscale && i >= 2 ? h : aoh, nullptr, 3, i < 2 ? packfilter : 1, i < 2 ? packformat : format, GL_TEXTURE_RECTANGLE);
        glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[i]);
        glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, aotex[i], 0);
        if(glCheckFramebufferStatus_(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            fatal("failed allocating AO buffer!");
        }
        if(!upscale && packformat == GL_RG16F)
        {
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }

    if(aoreducedepth && (aoreduce || aoreducedepth > 1))
    {
        if(!aotex[3])
        {
            glGenTextures(1, &aotex[3]);
        }
        if(!aofbo[3])
        {
            glGenFramebuffers_(1, &aofbo[3]);
        }
        createtexture(aotex[3], aow, aoh, nullptr, 3, 0, aodepthformat > 1 ? GL_R32F : (aodepthformat ? GL_R16F : GL_RGBA8), GL_TEXTURE_RECTANGLE);
        glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[3]);
        glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, aotex[3], 0);
        if(glCheckFramebufferStatus_(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            fatal("failed allocating AO buffer!");
        }
    }

    glBindFramebuffer_(GL_FRAMEBUFFER, 0);

    loadaoshaders();
    loadbilateralshaders();
}

void cleanupao()
{
    for(int i = 0; i < 4; ++i)
    {
        if(aofbo[i])
        {
            glDeleteFramebuffers_(1, &aofbo[i]);
            aofbo[i] = 0;
        }
    }
    for(int i = 0; i < 4; ++i)
    {
        if(aotex[i])
        {
            glDeleteTextures(1, &aotex[i]);
            aotex[i] = 0;
        }
    }
    if(aonoisetex)
    {
        glDeleteTextures(1, &aonoisetex);
        aonoisetex = 0;
    }
    aow = aoh = -1;

    clearaoshaders();
    clearbilateralshaders();
}

void initao()
{
    aodepthformat = aofloatdepth ? aofloatdepth : 0;
}

void viewao()
{
    if(!ao)
    {
        return;
    }
    int w = (debugfullscreen) ? hudw : min(hudw, hudh)/2; //if debugfullscreen, set to hudw/hudh size; if not, do small size
    int h = (debugfullscreen) ? hudh : (w*hudh)/hudw;
    SETSHADER(hudrect);
    gle::colorf(1, 1, 1);
    glBindTexture(GL_TEXTURE_RECTANGLE, aotex[2] ? aotex[2] : aotex[0]);
    int tw = aotex[2] ? gw : aow,
        th = aotex[2] ? gh : aoh;
    debugquad(0, 0, w, h, 0, 0, tw, th);
}

void renderao()
{
    if(!ao)
    {
        return;
    }
    timer *aotimer = begintimer("ambient obscurance");

    if(msaasamples)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msdepthtex);
    }
    else
    {
        glBindTexture(GL_TEXTURE_RECTANGLE, gdepthtex);
    }
    bool linear = aoreducedepth && (aoreduce || aoreducedepth > 1);
    float xscale = eyematrix.a.x,
          yscale = eyematrix.b.y;
    if(linear)
    {
        glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[3]);
        glViewport(0, 0, aow, aoh);
        SETSHADER(linearizedepth);
        screenquad(vieww, viewh);

        xscale *= static_cast<float>(vieww)/aow;
        yscale *= static_cast<float>(viewh)/aoh;

        glBindTexture(GL_TEXTURE_RECTANGLE, aotex[3]);
    }

    ambientobscuranceshader->set();

    glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[0]);
    glViewport(0, 0, aow, aoh);
    glActiveTexture_(GL_TEXTURE1);
    if(aoderivnormal)
    {
        if(msaasamples)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msdepthtex);
        }
        else
        {
            glBindTexture(GL_TEXTURE_RECTANGLE, gdepthtex);
        }
    }
    else
    {
        if(msaasamples)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msnormaltex);
        }
        else
        {
            glBindTexture(GL_TEXTURE_RECTANGLE, gnormaltex);
        }
        LOCALPARAM(normalmatrix, matrix3(cammatrix));
    }
    glActiveTexture_(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, aonoisetex);
    glActiveTexture_(GL_TEXTURE0);

    LOCALPARAMF(tapparams, aoradius*eyematrix.d.z/xscale, aoradius*eyematrix.d.z/yscale, aoradius*aoradius*aocutoff*aocutoff);
    LOCALPARAMF(contrastparams, (2.0f*aodark)/aotaps, aosharp);
    LOCALPARAMF(offsetscale, xscale/eyematrix.d.z, yscale/eyematrix.d.z, eyematrix.d.x/eyematrix.d.z, eyematrix.d.y/eyematrix.d.z);
    LOCALPARAMF(prefilterdepth, aoprefilterdepth);
    screenquad(vieww, viewh, aow/static_cast<float>(1<<aonoise), aoh/static_cast<float>(1<<aonoise));

    if(aobilateral)
    {
        if(aoreduce && aobilateralupscale)
        {
            for(int i = 0; i < 2; ++i)
            {
                setbilateralshader(aobilateral, i, aobilateraldepth);
                glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[i+1]);
                glViewport(0, 0, vieww, i ? viewh : aoh);
                glBindTexture(GL_TEXTURE_RECTANGLE, aotex[i]);
                glActiveTexture_(GL_TEXTURE1);
                if(msaasamples)
                {
                    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msdepthtex);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_RECTANGLE, gdepthtex);
                }
                glActiveTexture_(GL_TEXTURE0);
                screenquad(vieww, viewh, i ? vieww : aow, aoh);
            }
        }
        else
        {
            for(int i = 0; i < 2 + 2*aoiter; ++i)
            {
                setbilateralshader(aobilateral, i%2, aobilateraldepth);
                glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[(i+1)%2]);
                glViewport(0, 0, aow, aoh);
                glBindTexture(GL_TEXTURE_RECTANGLE, aotex[i%2]);
                glActiveTexture_(GL_TEXTURE1);
                if(linear)
                {
                    glBindTexture(GL_TEXTURE_RECTANGLE, aotex[3]);
                }
                else if(msaasamples)
                {
                    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msdepthtex);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_RECTANGLE, gdepthtex);
                }
                glActiveTexture_(GL_TEXTURE0);
                screenquad(vieww, viewh);
            }
        }
    }
    else if(aoblur)
    {
        float blurweights[maxblurradius+1],
              bluroffsets[maxblurradius+1];
        setupblurkernel(aoblur, blurweights, bluroffsets);
        for(int i = 0; i < 2+2*aoiter; ++i)
        {
            glBindFramebuffer_(GL_FRAMEBUFFER, aofbo[(i+1)%2]);
            glViewport(0, 0, aow, aoh);
            setblurshader(i%2, 1, aoblur, blurweights, bluroffsets, GL_TEXTURE_RECTANGLE);
            glBindTexture(GL_TEXTURE_RECTANGLE, aotex[i%2]);
            screenquad(aow, aoh);
        }
    }

    glBindFramebuffer_(GL_FRAMEBUFFER, msaasamples ? msfbo : gfbo);
    glViewport(0, 0, vieww, viewh);

    endtimer(aotimer);
}
