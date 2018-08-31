#include <stdio.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>


struct platform {
    void (*terminateGL)(void);
    void (*(*getProcAddress)(const char*))();
};
static struct platform gr3_platform_;
static void(*gr3_log_)(const char *);
static void(*gr3_appendtorenderpathstring_)(const char *);

/* OpenGL Context creation using GLX */

static Display *display; /*!< The used X display */
static Pixmap pixmap; /*!< The XPixmap (GLX < 1.4)*/
static GLXPbuffer pbuffer = (GLXPbuffer) NULL; /*!< The GLX Pbuffer (GLX >=1.4) */
static GLXContext context; /*!< The GLX context */


/*!
 * This function destroys the OpenGL context using GLX with a Pbuffer.
 */
void gr3_terminateGL_GLX_Pbuffer_(void) {
  gr3_log_("gr3_terminateGL_GLX_Pbuffer_();");
  
  glXMakeContextCurrent(display,None,None,NULL);
  glXDestroyContext(display, context);
  /*glXDestroyPbuffer(display, pbuffer);*/
  XCloseDisplay(display);
}

/*!
 * This function destroys the OpenGL context using GLX with a XPixmap.
 */
void gr3_terminateGL_GLX_Pixmap_(void) {
  gr3_log_("gr3_terminateGL_GLX_Pixmap_();");
  
  glXMakeContextCurrent(display,None,None,NULL);
  glXDestroyContext(display, context);
  XFreePixmap(display,pixmap);
  XCloseDisplay(display);
}

struct platform *gr3_platform_initGL_dynamic_(void(*log_callback)(const char *), void(*appendtorenderpathstring_callback)(const char *)) {

  int major = 0, minor = 0;
  int fbcount = 0;
  GLXFBConfig *fbc;
  GLXFBConfig fbconfig = (GLXFBConfig) NULL;
  pbuffer = (GLXPbuffer)NULL;

  gr3_log_ = log_callback;
  gr3_appendtorenderpathstring_ = appendtorenderpathstring_callback;
  gr3_log_("gr3_platform_initGL_dynamic_");
  gr3_platform_.getProcAddress = (void (*(*)(const char*))())&glXGetProcAddress;
  
  display = XOpenDisplay(0);
  if (!display) {
    gr3_log_("Not connected to an X server!");
    return NULL;
  }
  if (!glXQueryExtension(display, NULL, NULL)) {
    gr3_log_("GLX not supported!");
    return NULL;
  }
  
  context = glXGetCurrentContext();
  if (context != NULL) {
    gr3_appendtorenderpathstring_("GLX (existing context)");
  } else {
    /* call glXQueryVersion twice to prevent bugs in virtualbox */
    if (!glXQueryVersion(display,&major,&minor) && !glXQueryVersion(display,&major,&minor)) {
      return NULL;
    }
    if (major > 1 || minor >=4) {
      int i;
      int fb_attribs[] =
      {
        GLX_DRAWABLE_TYPE   , GLX_PBUFFER_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        None
      };
      int pbuffer_attribs[] =
      {
        GLX_PBUFFER_WIDTH   , 1,
        GLX_PBUFFER_HEIGHT   , 1,
        None
      };
      gr3_log_("(Pbuffer)");
      
      fbc = glXChooseFBConfig(display, DefaultScreen(display), fb_attribs,
                              &fbcount);
      if (fbcount == 0) {
        gr3_log_("failed to find a valid a GLX FBConfig for a RGBA PBuffer");
        XFree(fbc);
        XCloseDisplay(display);
        return NULL;
      }
      for (i = 0; i < fbcount && !pbuffer; i++) {
        fbconfig = fbc[i];
        pbuffer = glXCreatePbuffer(display, fbconfig, pbuffer_attribs);
      }
      XFree(fbc);
      if (!pbuffer) {
        gr3_log_("failed to create a RGBA PBuffer");
        XCloseDisplay(display);
        return NULL;
      }
      
      context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, None, True);
      glXMakeContextCurrent(display,pbuffer,pbuffer,context);
      gr3_platform_.terminateGL = &gr3_terminateGL_GLX_Pbuffer_;
      gr3_appendtorenderpathstring_("GLX (Pbuffer)");
    } else {
      XVisualInfo *visual;
      int fb_attribs[] =
      {
        GLX_DRAWABLE_TYPE   , GLX_PIXMAP_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        None
      };
      gr3_log_("(XPixmap)");
      fbc = glXChooseFBConfig(display, DefaultScreen(display), fb_attribs,
                              &fbcount);
      if (fbcount == 0) {
        gr3_log_("failed to find a valid a GLX FBConfig for a RGBA Pixmap");
        XFree(fbc);
        XCloseDisplay(display);
        return NULL;
      }
      fbconfig = fbc[0];
      XFree(fbc);
      
      context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, None, True);
      visual = glXGetVisualFromFBConfig(display,fbconfig);
      pixmap = XCreatePixmap(display,XRootWindow(display,DefaultScreen(display)),1,1,visual->depth);
      
      if (glXMakeContextCurrent(display,pixmap,pixmap,context)) {
        gr3_platform_.terminateGL = &gr3_terminateGL_GLX_Pixmap_;
        gr3_appendtorenderpathstring_("GLX (XPixmap)");
      } else {
        gr3_log_("failed to make GLX OpenGL Context current with a Pixmap");
        glXDestroyContext(display, context);
        XFreePixmap(display,pixmap);
        XCloseDisplay(display);
        return NULL;
      }
    }
  }
  return &gr3_platform_;
}
