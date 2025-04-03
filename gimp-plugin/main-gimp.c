/*
 * Originally Written 2003 by Lukas Kunc <Lukas.Kunc@seznam.cz>
 * Upgrade edits done 2025 by Joe Da Silva <digital@joescat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <glib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "compiler.h"
#include "hopfield.h"
#include "image.h"
#include "lambda.h"
#include "blur.h"
#include "gettext.h"

#define _(String) gettext (String)
#define gettext_noop(String) String
#define d_(String) String
#define N_(String) gettext_noop (String)

#define PREVIEW_SIZE		128
#define MOTION_ANGLE_DRA_SIZE	80
#define MOTION_ANGLE_DRA_MIDDLE	(MOTION_ANGLE_DRA_SIZE / 2)
#define MOTION_ANGLE_BUTTON	1
#define LAMBDAMIN_MAX		100.0
#define LAMBDAMIN_USABLE_MAX	0.999
#define LAMBDA_MAX		10000.0

#define RESPONSE_PREVIEW	1
#define RESPONSE_RESET		2

enum {
  BOUNDARY_MIRROR = 0,
  BOUNDARY_PERIODICAL,
  BOUNDARY_LAST
};

/* FORWARD DECLARATIONS */

static void query(void);
static void run(const gchar     *name,
                gint             nparams,
                const GimpParam *param,
                gint            *nreturn_vals,
                GimpParam      **returm_vals);
static void refocusit_help (const gchar *help_id,
                            gpointer help_data);

static void dialog_parameters_create();
static void dialog_parameters_init();
static void dialog_elements_update();
static void dialog_elements_destroy();
static void dialog_response(GtkWidget *widget, gint response_id, gpointer data);

static void input_parameters_init (void);
static void input_parameters_destroy (void);
static void input_parameters_load (void);
static void input_parameters_save (void);
static void input_parameters_fetch_params (const GimpParam *param);
static void input_parameters_fetch_dlg();
static int  image_parameters_init (const GimpParam *param, GimpParam *values);
static void image_parameters_destroy (void);
static int  hopfield_data_init (void);
static void hopfield_data_destroy (void);
static void hopfield_data_load (void);
static void hopfield_data_save (void);
static void preview_parameters_init (void);
static void preview_fetch_hopfield (void);
static void preview_update (void);
static void get_lambdas (gdouble* lambda, gdouble* lambda_min);
static int compute (int iterations);
static void motion_angle_draw (gboolean complete_redraw);
static void motion_angle_xy_calculate (gfloat x, gfloat y);

/* CONSTANTS */

static const char PROCEDURE_NAME[] =
  "plug-in-" PLUGIN_NAME;
static const char SHORT_DESCRIPTION[] = d_(
  "This plug-in iteratively refocuses a defocused image.");
static const char LONG_DESCRIPTION[] = d_(
  "The Iterative Refocus plug-in can sharpen images acquired by a defocused camera "
  "blurred with gaussian or motion blur or any combination of these degradations.\n\n"
  "Nice features are adaptive/static area smoothing to remove 'ringing' effect "
  "introduced by image edges and noise.\n\n"
  "NOT nice features of this plug-in are memory and CPU requirements.\n\n"
  "Iterative Refocus uses Hopfield Neural Network algorithm to find minimum error.");

const GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

/* DATA STRUCTURES */

typedef struct {
  char         *name;
  GtkWidget    *menu_item;
} SListbox;

typedef void (*FListboxHandler)(GtkWidget*, guint);

typedef struct {
  GtkWidget    *preview;
  guint         x;
  guint         y;
  guint         width;
  guint         height;
  guint         size;
  guchar       *data;
  gdouble      *linear;
  const Babl   *rgb8;
} SPreview;

typedef struct {
  gdouble       radius;
  gdouble       gauss;
  gdouble       motion;
  gdouble       mot_angle;
  gdouble       lambda;
  gdouble       lambda_min;
  guint         winsize;
  guint         iterations;
  guint         boundary;
  guint         adaptive_smooth;
  guint         prev_iter;
} SInputParameters;

typedef struct {
  guint         sel_width;
  guint         sel_height;
  gint          sel_x1;
  gint          sel_y1;
  gint          sel_x2;
  gint          sel_y2;
  gint          img_bpp;
  guint         size;
  gboolean      gray;
  gboolean      rgb;
  GimpDrawable *drawable;
  GeglBuffer   *srcBuf;
  GeglBuffer   *destBuf;
  gdouble      *srcImg;
  guchar       *destImg;
  const Babl   *format;
  const Babl   *linear;
  gint          xImg;
  gint          yImg;
  gint          bppImg;
} SImageParameters;

typedef struct {
  GtkAdjustment *radius;
  GtkAdjustment *gauss;
  GtkAdjustment *motion;
  GtkAdjustment *mot_angle;
  GtkAdjustment *lambda;
  GtkAdjustment *lambda_min;
  GtkAdjustment *winsize;
  GtkAdjustment *iterations;
  GtkAdjustment *prev_iter;
  GtkAdjustment *hscroll;
  GtkAdjustment *vscroll;
  gboolean       frun;
  gboolean       finish;
  gboolean       area_smooth_enabled;
} SDialogParameters;

typedef struct {
  GtkWidget* progress;
  GtkWidget* motion_angle_dra;
  GtkWidget* adaptive;
  GtkWidget* area_smooth;
  GtkWidget* boundary;
  GtkWidget* dialog;
} SDialogElements;

typedef struct {
  image_t    imageR;
  image_t    imageG;
  image_t    imageB;
  hopfield_t hopfieldR;
  hopfield_t hopfieldG;
  hopfield_t hopfieldB;
  convmask_t blur;
  convmask_t filter;
  lambda_t   lambdafldR;
  lambda_t   lambdafldG;
  lambda_t   lambdafldB;
} SHopfield;

/* STATIC DATA */

static SDialogElements    dialog_elements;
static SDialogParameters  dialog_parameters;
static SInputParameters   input_parameters;
static SImageParameters   image_parameters;
static SPreview           preview;
static SHopfield          hopfield;
static SListbox           boundary_listbox[BOUNDARY_LAST + 1];

/* CALLBACKS */

static void no_smooth_callback (GtkWidget *widget, gpointer data) {
  if (dialog_elements.area_smooth) {
    if (dialog_parameters.lambda->value < 1e-6) {
      gtk_widget_set_sensitive (GTK_WIDGET (dialog_elements.area_smooth), FALSE);
      dialog_parameters.area_smooth_enabled = FALSE;
    } else {
      if (!dialog_parameters.area_smooth_enabled) {
        gtk_widget_set_sensitive (GTK_WIDGET (dialog_elements.area_smooth), TRUE);
        dialog_parameters.area_smooth_enabled = TRUE;
      }
    }
  }
}

static void motion_vector_change_callback (GtkWidget *widget, gpointer data) {
  motion_angle_draw(FALSE);
}

static gboolean motion_vector_expose_callback (GtkWidget *widget, GdkEvent* gdkev, gpointer data) {
  motion_angle_draw(TRUE);
  return FALSE;
}

static gboolean motion_vector_mouse_move_callback (GtkWidget *widget, GdkEvent* gdkev, gpointer data) {
  motion_angle_xy_calculate(gdkev->motion.x, gdkev->motion.y);
  return FALSE;
}

static gboolean motion_vector_mouse_release_callback (GtkWidget *widget, GdkEvent* gdkev, gpointer data) {
  if (gdkev->button.button == MOTION_ANGLE_BUTTON) {
    gtk_signal_disconnect_by_func (GTK_OBJECT (widget), GTK_SIGNAL_FUNC (motion_vector_mouse_release_callback), NULL);
    gtk_signal_disconnect_by_func (GTK_OBJECT (widget), GTK_SIGNAL_FUNC (motion_vector_mouse_move_callback), NULL);
  }
  return FALSE;
}

static gboolean motion_vector_mouse_press_callback (GtkWidget *widget, GdkEvent* gdkev, gpointer data) {
  if (gdkev->button.button == MOTION_ANGLE_BUTTON) {
    motion_angle_xy_calculate (gdkev->button.x, gdkev->button.y);
    gtk_signal_connect (GTK_OBJECT (widget), "button-release-event", GTK_SIGNAL_FUNC (motion_vector_mouse_release_callback), NULL);
    gtk_signal_connect (GTK_OBJECT (widget), "motion-notify-event", GTK_SIGNAL_FUNC (motion_vector_mouse_move_callback), NULL);
  }
  return FALSE;
}

static void boundary_callback (GtkWidget* menu_item, guint index) {
  input_parameters.boundary = (guchar)index;
}

static void destroy_callback (GtkWidget *widget, gpointer data) {
  dialog_parameters.finish = TRUE;
  gtk_widget_destroy (dialog_elements.dialog);
  dialog_elements_destroy ();
  gtk_main_quit ();
}

static void ok_callback (GtkWidget *widget, gpointer data) {
  gtk_widget_set_sensitive (dialog_elements.dialog, FALSE);
  input_parameters_fetch_dlg ();
  if (compute (input_parameters.iterations))
    hopfield_data_save ();
  gtk_widget_set_sensitive (dialog_elements.dialog, TRUE);
  gtk_widget_destroy (dialog_elements.dialog);
  dialog_parameters.frun = TRUE;
  dialog_elements_destroy ();
  gtk_main_quit ();
}

static void defaults_callback (GtkWidget *widget, gpointer data) {
  input_parameters_init ();
  dialog_parameters_init ();
  dialog_elements_update ();
  hopfield_data_load ();
  preview_update ();
}

static void preview_callback (GtkWidget *widget, gpointer data) {
  gtk_widget_set_sensitive (dialog_elements.dialog, FALSE);
  input_parameters_fetch_dlg ();
  compute (input_parameters.prev_iter);
  gtk_widget_set_sensitive (dialog_elements.dialog, TRUE);
}

static void preview_scroll_callback (GtkWidget *widget, gpointer data) {
  preview.x = (guint)dialog_parameters.hscroll->value;
  preview.y = (guint)dialog_parameters.vscroll->value;
  preview_update ();
}

/* FUNCTIONS */

MAIN ()

static void query (void) {
  static GimpParamDef args[] = {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable to modify"},
    {GIMP_PDB_FLOAT, "radius", "Blur radius (default = 6.0)"},
    {GIMP_PDB_FLOAT, "gauss", "Gaussian blur variance (default = 0.0)"},
    {GIMP_PDB_FLOAT, "motion", "Motion size (default = 0.0)"},
    {GIMP_PDB_FLOAT, "mot_angle", "Motion angle (default = 0.0)"},
    {GIMP_PDB_FLOAT, "lambda", "Noise reduction (default = 100.0)"},
    {GIMP_PDB_INT32, "boundary", "Boundary conditions (default = mirror / 0)"},
    {GIMP_PDB_FLOAT, "lambda_min", "Area smoothnes (default = 30.0)"},
    {GIMP_PDB_INT32, "adaptive_smooth", "Adaptive smoothing (default = TRUE)"},
    {GIMP_PDB_INT32, "winsize", "Smooth area size (default = 3)"},
    {GIMP_PDB_INT32, "iterations", "Number of iterations (default = 100)"},
    {GIMP_PDB_INT32, "prev_iter", "Number of iterations for preview (default = 10)"}
  };

#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);
#endif

  gimp_install_procedure (PROCEDURE_NAME, _(SHORT_DESCRIPTION), _(LONG_DESCRIPTION),
  /* copyright author  */ "Lukas Kunc, 2004, <Lukas.Kunc@seznam.cz>",
  /* copyright license */ "GPL3+",
  /* build date        */ "2025",
  /* menu entry        */ _("Iterative refocus..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, 0);
  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);
  gimp_plugin_menu_register (PROCEDURE_NAME, "<Image>/Filters/Enhance");
}

static void input_parameters_destroy (void) {
}

static void input_parameters_init (void) {
  input_parameters.radius = 6.0;
  input_parameters.gauss = 0.0;
  input_parameters.motion = 0.0;
  input_parameters.mot_angle = 0.0;
  input_parameters.lambda = 100.0;
  input_parameters.lambda_min = 30.0;
  input_parameters.winsize = 3;
  input_parameters.iterations = 100;
  input_parameters.prev_iter = 10;
  input_parameters.boundary = BOUNDARY_MIRROR;
  input_parameters.adaptive_smooth = TRUE;
}

static void input_parameters_load (void) {
  gimp_get_data (PACKAGE_NAME, &input_parameters);
}

static void input_parameters_save (void) {
  gimp_set_data (PACKAGE_NAME, &input_parameters, sizeof (input_parameters));
}

static void input_parameters_fetch_params (const GimpParam *param) {
  input_parameters.radius          = param[3].data.d_float;
  input_parameters.gauss           = param[4].data.d_float;
  input_parameters.motion          = param[5].data.d_float;
  input_parameters.mot_angle       = param[6].data.d_float;
  input_parameters.lambda          = param[7].data.d_float;
  input_parameters.boundary        = param[8].data.d_int32;
  input_parameters.lambda_min      = param[9].data.d_float;
  input_parameters.adaptive_smooth = param[10].data.d_int32;
  input_parameters.winsize         = param[11].data.d_int32;
  input_parameters.iterations      = param[12].data.d_int32;
  input_parameters.prev_iter       = param[13].data.d_int32;
}

static void input_parameters_fetch_dlg () {
  input_parameters.radius          = dialog_parameters.radius->value;
  input_parameters.gauss           = dialog_parameters.gauss->value;
  input_parameters.motion          = dialog_parameters.motion->value;
  input_parameters.mot_angle       = dialog_parameters.mot_angle->value;
  input_parameters.lambda          = dialog_parameters.lambda->value;
  input_parameters.lambda_min      = dialog_parameters.lambda_min->value;
  input_parameters.winsize         = (guint) dialog_parameters.winsize->value;
  input_parameters.iterations      = (guint) dialog_parameters.iterations->value;
  input_parameters.prev_iter       = (guint) dialog_parameters.prev_iter->value;
  /* no action for boundary - updated automatically */
  input_parameters.adaptive_smooth = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog_elements.adaptive));
}

static void dialog_parameters_init () {
  gtk_adjustment_set_value(dialog_parameters.radius,     (gfloat)input_parameters.radius);
  gtk_adjustment_set_value(dialog_parameters.gauss,      (gfloat)input_parameters.gauss);
  gtk_adjustment_set_value(dialog_parameters.motion,     (gfloat)input_parameters.motion);
  gtk_adjustment_set_value(dialog_parameters.mot_angle,  (gfloat)input_parameters.mot_angle);
  gtk_adjustment_set_value(dialog_parameters.lambda,     (gfloat)input_parameters.lambda);
  gtk_adjustment_set_value(dialog_parameters.lambda_min, (gfloat)input_parameters.lambda_min);
  gtk_adjustment_set_value(dialog_parameters.winsize,    (gfloat)input_parameters.winsize);
  gtk_adjustment_set_value(dialog_parameters.iterations, (gfloat)input_parameters.iterations);
  gtk_adjustment_set_value(dialog_parameters.prev_iter,  (gfloat)input_parameters.prev_iter);
  dialog_parameters.area_smooth_enabled = TRUE;
}

static void dialog_parameters_create () {
  dialog_parameters.frun = FALSE;
  dialog_parameters.finish = FALSE;

  dialog_parameters.radius     = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.radius, 0.0f, 32.0f, 0.01f, 0.1f, 0.0f));
  dialog_parameters.gauss      = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.gauss, 0.0f, 32.0f, 0.01f, 0.1f, 0.0f));
  dialog_parameters.motion     = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.motion, 0.0f, 32.0f, 0.01f, 0.1f, 0.0f));
  dialog_parameters.mot_angle  = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.motion, 0.0f, 360.0f, 0.01f, 0.1f, 0.0f));
  dialog_parameters.lambda     = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.lambda, 0.0f, (gfloat)LAMBDA_MAX, 0.1f, 1.0f, 0.0f));
  dialog_parameters.lambda_min = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.lambda_min, 0.0f, (gfloat)LAMBDAMIN_MAX, 0.1f, 1.0f, 0.0f));
  dialog_parameters.winsize    = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.winsize, 1.0f, 16.0f, 1.0f, 1.0f, 0.0f));
  dialog_parameters.iterations = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.iterations, 1.0f, 200.0f, 1.0f, 10.0f, 0.0f));
  dialog_parameters.prev_iter  = GTK_ADJUSTMENT (gtk_adjustment_new ((gfloat)input_parameters.prev_iter, 1.0f, 20.0f, 1.0f, 1.0f, 0.0f));
  dialog_parameters.hscroll    = GTK_ADJUSTMENT (gtk_adjustment_new (0.0f, 0.0f, (gfloat)image_parameters.sel_width - 1.0f, 1.0f, (gfloat)preview.width, (gfloat)preview.width));
  dialog_parameters.vscroll    = GTK_ADJUSTMENT (gtk_adjustment_new (0.0f, 0.0f, (gfloat)image_parameters.sel_height - 1.0f, 1.0f, (gfloat)preview.height, (gfloat)preview.height));

  gtk_signal_connect (GTK_OBJECT (dialog_parameters.mot_angle), "value_changed", GTK_SIGNAL_FUNC (motion_vector_change_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dialog_parameters.lambda), "value_changed", GTK_SIGNAL_FUNC (no_smooth_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dialog_parameters.hscroll), "value_changed", GTK_SIGNAL_FUNC (preview_scroll_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dialog_parameters.vscroll), "value_changed", GTK_SIGNAL_FUNC (preview_scroll_callback), NULL);

  boundary_listbox[BOUNDARY_MIRROR].name = _("mirror boundary");
  boundary_listbox[BOUNDARY_PERIODICAL].name = _("periodical boundary");
  boundary_listbox[BOUNDARY_LAST].name = NULL;
}

static void dialog_elements_update () {
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_elements.adaptive), input_parameters.adaptive_smooth);
  gtk_option_menu_set_history (GTK_OPTION_MENU (dialog_elements.boundary), input_parameters.boundary);
  if (dialog_elements.area_smooth && dialog_parameters.lambda->value < 1e-6) {
    gtk_widget_set_sensitive (GTK_WIDGET (dialog_elements.area_smooth), FALSE);
    dialog_parameters.area_smooth_enabled = FALSE;
  }
  motion_angle_draw (FALSE);
}

static void dialog_elements_destroy () {
  dialog_elements.progress    = NULL;
  dialog_elements.adaptive    = NULL;
  dialog_elements.area_smooth = NULL;
  dialog_elements.boundary    = NULL;
  dialog_elements.dialog      = NULL;
}


static void dialog_response (GtkWidget *widget, gint response_id, gpointer data) {
  switch (response_id) {
  case RESPONSE_PREVIEW:
    preview_callback (widget, data);
    break;
  case GTK_RESPONSE_OK:
    ok_callback (widget, data);
    break;
  case RESPONSE_RESET:
    defaults_callback (widget, data);
    break;
  case GTK_RESPONSE_CANCEL:
  default:
    destroy_callback (widget, data);
    break;
  }
}

static int image_parameters_init (const GimpParam *param, GimpParam *values) {
  image_parameters.drawable   = gimp_drawable_get (param[2].data.d_drawable);
  image_parameters.rgb        = gimp_drawable_is_rgb (image_parameters.drawable->drawable_id);
  image_parameters.gray       = gimp_drawable_is_gray (image_parameters.drawable->drawable_id);
  if (!(image_parameters.rgb || image_parameters.gray)) {
#if defined(NDEBUG)
    printf("Error, image_parameters_init() - image is not rgb or gray!\n");
#endif
    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    return -1;
  }

  gimp_drawable_mask_bounds (image_parameters.drawable->drawable_id,
                             &image_parameters.sel_x1, &image_parameters.sel_y1,
                             &image_parameters.sel_x2, &image_parameters.sel_y2);

  image_parameters.sel_width  = image_parameters.sel_x2 - image_parameters.sel_x1;
  image_parameters.sel_height = image_parameters.sel_y2 - image_parameters.sel_y1;
  image_parameters.img_bpp    = gimp_drawable_bpp (image_parameters.drawable->drawable_id);
  image_parameters.size       = image_parameters.sel_width * image_parameters.sel_height;

  preview.data = preview.linear = NULL;
  return 0;
}

static void image_parameters_destroy (void) {
  gimp_drawable_detach (image_parameters.drawable);
  if (preview.data)   g_free(preview.data);
  if (preview.linear) g_free(preview.linear);
}

static void preview_parameters_init (void) {
  const char *str, *ret;

#if defined(NDEBUG)
    printf("preview_parameters_init() - starting!\n");
#endif
  /* select and RGB version of existing format */
  str = babl_get_name (image_parameters.format);
  if (strstr(str, "RaGaBa") || strstr(str, "YaA ") || strstr(str, "Ya "))
    ret = "RaGaBa u8";
  else if (strstr(str, "R'G'B'") || strstr(str, "Y'A ") || strstr(str, "Y' "))
    ret = "R'G'B' u8";
  else if (strstr(str, "R~G~B~") || strstr(str, "Y~A ") || strstr(str, "Y~ "))
    ret = "R~G~B~ u8";
  else if (strstr(str, "R'aG'aB'a") || strstr(str, "Y'aA ") || strstr(str, "Y'a "))
    ret = "R'aG'aB'a u8";
  else //if (strstr(str, "RGB") || strstr(str, "YA ") || strstr(str, "Y "))
    ret = "RGB u8";
#if defined(NDEBUG)
    printf("preview_parameters_init() - found <%s> and will output <%s>\n", str, ret);
#endif
  //g_free (str);

  preview.width  = MIN (image_parameters.sel_width, PREVIEW_SIZE);
  preview.height = MIN (image_parameters.sel_height, PREVIEW_SIZE);
  preview.x = preview.y = 0;
  preview.size   = preview.width * preview.height;
  preview.rgb8   = babl_format (ret);
  preview.data   = g_new (guchar,  preview.size * 3);
  preview.linear = g_new (gdouble, preview.size * (image_parameters.rgb ? 3:1));
}

static int hopfield_data_init (void) {
  gint32      drawable_ID;
  gint        xImg, yImg, pixelCount, bppImg;

  gegl_init (NULL, NULL);

  drawable_ID = image_parameters.drawable->drawable_id;
  image_parameters.format = gimp_drawable_get_format (drawable_ID);
  image_parameters.bppImg = bppImg = babl_format_get_bytes_per_pixel (image_parameters.format);

  /* Load 'linear_double RGB' or 'linear_double Gray' into srcImg */
  image_parameters.xImg = xImg = gimp_drawable_width(drawable_ID);
  image_parameters.yImg = yImg = gimp_drawable_height(drawable_ID);
  pixelCount = xImg * yImg;
  if (!(image_parameters.srcImg = g_new (gdouble, pixelCount * (image_parameters.rgb ? 3:1))))
    goto hopfield_data_init_err0;
  if (!(image_parameters.destImg = g_new (guchar, pixelCount * bppImg)))
    goto hopfield_data_init_err1;

  if (!(image_parameters.srcBuf = gimp_drawable_get_buffer (drawable_ID))) {
    goto hopfield_data_init_err2;
  }
  gegl_buffer_get (image_parameters.srcBuf, GEGL_RECTANGLE(0, 0, xImg, yImg), 1.0, \
                   image_parameters.format, image_parameters.destImg, \
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  image_parameters.linear = babl_format ((image_parameters.rgb ? "RGB double":"Y double"));
  babl_process (babl_fish (image_parameters.format, image_parameters.linear), \
                image_parameters.destImg, image_parameters.srcImg, \
                pixelCount);
#if defined(NDEBUG)
  printf("hopfield_data_init()\nDrawable image format <%s>, bytes per pixel=%d, xImg=%d yImg=%d\n",
         babl_get_name (image_parameters.format), image_parameters.bppImg, xImg, yImg);
  for (int i = 0; i <40; i++) {
    printf("|%d-%d-%f",i,image_parameters.destImg[i],image_parameters.srcImg[i]);
  }
  printf("\nBuffer srcImg format <%s>\n", babl_get_name (image_parameters.linear));
#endif
  g_object_unref (image_parameters.srcBuf);

  /* init hopfield data */
  if (!(image_create (&hopfield.imageR, image_parameters.sel_width, image_parameters.sel_height)))
    goto hopfield_data_init_err3;
  if (image_parameters.rgb) {
    if (!(image_create (&hopfield.imageG, image_parameters.sel_width, image_parameters.sel_height)))
      goto hopfield_data_init_err4;
    if (!(image_create (&hopfield.imageB, image_parameters.sel_width, image_parameters.sel_height)))
      goto hopfield_data_init_err5;
  }
  return 0;

/* Out of memory if you are here */
hopfield_data_init_err5:
  image_destroy (&hopfield.imageG);
hopfield_data_init_err4:
  image_destroy (&hopfield.imageR);
hopfield_data_init_err3:
hopfield_data_init_err2:
  g_free (image_parameters.destImg);
hopfield_data_init_err1:
  g_free (image_parameters.srcImg);
hopfield_data_init_err0:
  gegl_exit ();
#if defined(NDEBUG)
  printf("Error, hopfield_data_init() - out of memory!\n");
#endif
  return -1;
}

static void hopfield_data_destroy (void) {
  if (image_parameters.rgb) {
    image_destroy (&hopfield.imageB);
    image_destroy (&hopfield.imageG);
  }
  image_destroy (&hopfield.imageR);

  g_free (image_parameters.destImg);
  g_free (image_parameters.srcImg);
}

static void hopfield_data_save (void) {
  gint32      drawable_ID;
  gdouble    *ptr;
  gint        x, y, xImg, yImg;

  drawable_ID = image_parameters.drawable->drawable_id;
  xImg = image_parameters.xImg;
  yImg = image_parameters.yImg;

  ptr = image_parameters.srcImg;
  if (image_parameters.rgb) {
    for (y = 0; y < yImg; y++) {
      for (x = 0; x < xImg; x++) {
        *(ptr++) = MIN (image_get (&hopfield.imageR, x, y), 1.0);
        *(ptr++) = MIN (image_get (&hopfield.imageG, x, y), 1.0);
        *(ptr++) = MIN (image_get (&hopfield.imageB, x, y), 1.0);
      }
    }
  } else {
    for (y = 0; y < yImg; y++) {
      for (x = 0; x < xImg; x++) {
        *(ptr++) = MIN (image_get (&hopfield.imageR, x, y), 1.0);
      }
    }
  }

  babl_process (babl_fish (image_parameters.linear, image_parameters.format), \
                image_parameters.srcImg, image_parameters.destImg, \
                (xImg * yImg));
#if defined(NDEBUG)
  printf("hopfield_data_save() - converted srcImg back to drawable format!\n");
  printf("Drawable image format <%s>, bytes per pixel=%d, xImg=%d yImg=%d\n",
         babl_get_name (image_parameters.format), image_parameters.bppImg, xImg, yImg);
  for (int i = 0; i <40; i++) {
    printf("|%d-%f-%d",i,image_parameters.srcImg[i],image_parameters.destImg[i]);
  }
  printf("\nBuffer srcImg format <%s>\n", babl_get_name (image_parameters.linear));
#endif

  /* merge the shadow, update the drawable */
  if (!(image_parameters.destBuf = gimp_drawable_get_shadow_buffer (drawable_ID)))
    goto hopfield_data_save_err0;
  gegl_buffer_set (image_parameters.destBuf, GEGL_RECTANGLE(0, 0, xImg, yImg), 0, \
                   image_parameters.format, image_parameters.destImg, GEGL_AUTO_ROWSTRIDE);
  g_object_unref (image_parameters.destBuf);
  gimp_drawable_merge_shadow (drawable_ID, TRUE);
#if defined(NDEBUG)
  printf("hopfield_data_save() - shadow merged!\n");
#endif
  gimp_drawable_update (drawable_ID, 0, 0, xImg, yImg);
  return;

hopfield_data_save_err0:
#if defined(NDEBUG)
  printf("Error, hopfield_data_save() - out of memory!\n");
#endif
  return;

}

static void hopfield_data_load (void) {
  guint    x, y;
  gdouble *ptr;

  ptr = image_parameters.srcImg;
  if (image_parameters.rgb) {
    for (y = 0; y < image_parameters.yImg; y++) {
      for (x = 0; x < image_parameters.xImg; x++) {
        image_set (&hopfield.imageR, x, y, *ptr);
        ptr++;
        image_set (&hopfield.imageG, x, y, *ptr);
        ptr++;
        image_set (&hopfield.imageB, x, y, *ptr);
        ptr++;
      }
    }
  } else {
    for (y = 0; y < image_parameters.sel_height; y++) {
      for (x = 0; x < image_parameters.sel_width; x++) {
        image_set (&hopfield.imageR, x, y, *ptr);
        ptr++;
      }
    }
  }
}

static void preview_fetch_hopfield (void) {
  guint    x, y;
  guint    w, h;
  gdouble *ptr;

  w = preview.width + preview.x;
  h = preview.height + preview.y;
  ptr = preview.linear;

  if (image_parameters.rgb) {
    for (y = preview.y; y < h; y++) {
      for (x = preview.x; x < w; x++) {
        *(ptr++) = image_get (&hopfield.imageR, x, y);
        *(ptr++) = image_get (&hopfield.imageG, x, y);
        *(ptr++) = image_get (&hopfield.imageB, x, y);
      }
    }
  } else {
    for (y = preview.y; y < h; y++) {
      for (x = preview.x; x < w; x++) {
        *(ptr++) = image_get (&hopfield.imageR, x, y);
      }
    }
  }
  babl_process (babl_fish (image_parameters.linear, preview.rgb8), \
                preview.linear, preview.data, preview.size);
}

static void preview_update (void) {
  guint   y;
  guchar *image;

  preview_fetch_hopfield ();

  for (y = 0, image = preview.data; y < preview.height; y ++, image += preview.width * 3) {
    gtk_preview_draw_row (GTK_PREVIEW (preview.preview), image, 0, y, preview.width);
  }

  gtk_widget_draw (preview.preview, NULL);
  gdk_flush ();
}

/* GUI ELEMENTS */

static GtkWidget* scaler_new (GtkAdjustment* adj, gfloat climb_rate, guint digits) {
  GtkWidget *box;
  GtkWidget *element;

  box = gtk_hbox_new (TRUE, 2);

  /* hscale */
  element = gtk_hscale_new (adj);
  gtk_scale_set_digits (GTK_SCALE (element), digits);
  gtk_scale_set_draw_value (GTK_SCALE (element), FALSE);
  gtk_box_pack_start (GTK_BOX (box), element, TRUE, TRUE, 0);
  gtk_widget_show (element);

  /* spin button */
  element = gtk_spin_button_new (adj, climb_rate, digits);
  gtk_box_pack_start (GTK_BOX (box), element, TRUE, TRUE, 0);
  gtk_widget_show (element);

  return box;
}

static GtkWidget* listbox_new (SListbox* listdef, FListboxHandler handler, guint active) {
  GtkWidget* element;
  GtkWidget* listbox;
  GtkWidget* menu;
  guint      item;

  listbox = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  item = 0;
  while (listdef->name) {
    element = gtk_menu_item_new_with_label (listdef->name);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), element);
    gtk_signal_connect (GTK_OBJECT (element), "activate", GTK_SIGNAL_FUNC (handler), GUINT_TO_POINTER (item));
    gtk_widget_show (element);
    listdef->menu_item = element;
    listdef++; item++;
  }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (listbox), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (listbox), active);
  return listbox;
}

static GtkWidget* create_degradation_params () {
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *element;

  frame = gtk_frame_new (_("Degradation"));

  table = gtk_table_new (2, 7, FALSE);

  /* blur radius */
  element = gtk_label_new (_("Radius:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 0, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.radius, 0.01f, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 0, 1);
  gtk_widget_show (element);

  /* gaussian blur */
  element = gtk_label_new (_("Gauss:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 1, 2);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.gauss, 0.01f, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 1, 2);
  gtk_widget_show (element);

  /* motion blur */
  element = gtk_label_new (_("Motion size:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 2, 3);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.motion, 0.01f, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 2, 3);
  gtk_widget_show (element);

  /* motion angle */
  element = gtk_label_new (_("Motion angle:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 3, 4);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.mot_angle, 0.01f, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 3, 4);
  gtk_widget_show (element);

  /* noise reduction */
  element = gtk_label_new (_("Noise:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 4, 5);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.lambda, 0.1f, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 4, 5);
  gtk_widget_show (element);

  /* iterations */
  element = gtk_label_new (_("Iterations:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 5, 6);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.iterations, 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 5, 6);
  gtk_widget_show (element);

  /* boundary */
  element = gtk_label_new (_("Boundary:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 6, 7);
  gtk_widget_show (element);

  element = dialog_elements.boundary = listbox_new (boundary_listbox, boundary_callback, input_parameters.boundary);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 6, 7);
  gtk_widget_show (element);

  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (frame);
  return frame;
}

static GtkWidget* create_area_params () {
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *element;

  frame = gtk_frame_new (_("Area smoothing"));

  table = gtk_table_new (3, 2, FALSE);

  element = gtk_label_new (_("Smoothness:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 0, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.lambda_min, 1.0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 0, 1);
  gtk_widget_show (element);

  element = gtk_label_new (_("Area size:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 1, 2);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.winsize, 0.0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 1, 2);
  gtk_widget_show (element);

  element = gtk_label_new (_("Adaptive smoothing:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 0, 1, 2, 3);
  gtk_widget_show (element);

  element = dialog_elements.adaptive = gtk_check_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (element), input_parameters.adaptive_smooth);
  gtk_table_attach_defaults (GTK_TABLE (table), element, 1, 2, 2, 3);
  gtk_widget_show (element);

  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_widget_show (table);

  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (frame);
  return frame;
}

static GtkWidget* create_controls () {
  GtkWidget *vbox;
  GtkWidget *element;

  vbox = gtk_vbox_new (FALSE, 0);

  /* blur params */
  element = create_degradation_params ();
  gtk_box_pack_start (GTK_BOX (vbox), element, FALSE, FALSE, 0);
  gtk_widget_show (element);

  /* area params */
  element = dialog_elements.area_smooth = create_area_params ();
  gtk_box_pack_start (GTK_BOX (vbox), element, FALSE, FALSE, 0);
  gtk_widget_show (element);

  /* progress */
  element = dialog_elements.progress = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), element, FALSE, FALSE, 0);
  gtk_widget_show (element);

  gtk_widget_show (vbox);

  return vbox;
}

static void motion_angle_draw (gboolean complete_redraw) {
  static gint ox = MOTION_ANGLE_DRA_MIDDLE, oy = MOTION_ANGLE_DRA_MIDDLE;
  gdouble x, y, a;

  if (dialog_elements.motion_angle_dra) {
    if (complete_redraw) {
      gdk_draw_arc (dialog_elements.motion_angle_dra->window, dialog_elements.motion_angle_dra->style->black_gc, TRUE,
                    0, 0, MOTION_ANGLE_DRA_SIZE, MOTION_ANGLE_DRA_SIZE, 0, 360*64);
    }
    a = dialog_parameters.mot_angle->value * M_PI / 180.0;
    x = y = MOTION_ANGLE_DRA_MIDDLE - 1;
    x *= cos(a);
    y *= sin(a);
    gdk_draw_line (dialog_elements.motion_angle_dra->window, dialog_elements.motion_angle_dra->style->black_gc,
                   MOTION_ANGLE_DRA_MIDDLE, MOTION_ANGLE_DRA_MIDDLE, ox, oy);

    ox = MOTION_ANGLE_DRA_MIDDLE + (gint) (x+0.5);
    oy = MOTION_ANGLE_DRA_MIDDLE - (gint) (y+0.5);

    gdk_draw_line (dialog_elements.motion_angle_dra->window, dialog_elements.motion_angle_dra->style->white_gc,
                   MOTION_ANGLE_DRA_MIDDLE, MOTION_ANGLE_DRA_MIDDLE, ox, oy);
    gdk_flush ();
  }
}

static GtkWidget* motion_angle_create () {
  GtkWidget *frame;
  GtkWidget *box;
  GtkWidget *element;

  box = gtk_hbox_new (FALSE, 0);

  frame = gtk_frame_new (_("Motion direction"));
  element = dialog_elements.motion_angle_dra = gtk_drawing_area_new ();
  gtk_widget_set_usize (element, MOTION_ANGLE_DRA_SIZE, MOTION_ANGLE_DRA_SIZE);
  gtk_widget_show (element);

  gtk_box_pack_start (GTK_BOX (box), element, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_widget_show (box);

  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (frame);

  gtk_widget_add_events (element, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
  gtk_signal_connect (GTK_OBJECT (element), "expose-event", GTK_SIGNAL_FUNC (motion_vector_expose_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (element), "button-press-event", GTK_SIGNAL_FUNC (motion_vector_mouse_press_callback), NULL);

  return frame;
}

static void motion_angle_xy_calculate (gfloat x, gfloat y) {
  gfloat r, a;

  x -= MOTION_ANGLE_DRA_MIDDLE; 
  y = MOTION_ANGLE_DRA_MIDDLE - y;
  r = sqrt(x*x + y*y);
  if (r < 1e-4) a = 0.0;
  else {
    a = acos(x/r) * 180.0 / M_PI;
    if (y < 0.0) a = 360.0 - a;
  }
  gtk_adjustment_set_value (dialog_parameters.mot_angle, a);
}

static GtkWidget* preview_create () {
  GtkWidget *frame;
  GtkWidget *vbox, *hbox;
  GtkWidget *element;
  GtkWidget *scrollbar;
  GtkWidget *table;

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);

  gtk_table_set_col_spacings (GTK_TABLE (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);

  /* preview */
  element = preview.preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (element), preview.width, preview.height);
  gtk_table_attach (GTK_TABLE (table), element, 0, 1, 0, 1, 0, 0, 0, 0);
  gtk_widget_show (element);

  scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (dialog_parameters.hscroll));
  gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (table), scrollbar, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (scrollbar);

  scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (dialog_parameters.vscroll));
  gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (table), scrollbar, 1, 2, 0, 1, 0, GTK_FILL, 0, 0);
  gtk_widget_show (scrollbar);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* iterations */
  hbox = gtk_hbox_new (FALSE, 2);
  element = gtk_label_new (_("Iterations:"));
  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), element, FALSE, FALSE, 0);
  gtk_widget_show (element);

  element = gtk_hscale_new (dialog_parameters.prev_iter);
  gtk_scale_set_digits (GTK_SCALE (element), 0);
  gtk_box_pack_start (GTK_BOX (hbox), element, TRUE, TRUE, 0);
  gtk_widget_show (element);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Preview"));
  gtk_container_add(GTK_CONTAINER (frame), vbox);
  gtk_widget_show (frame);
  return frame;
}

static gboolean dialog (void) {
  GtkWidget *element;
  GtkWidget *hbox;
  GtkWidget *dlg;
  GtkWidget *vbox;

  dialog_elements.dialog = dlg = gimp_dialog_new (_("Iterative Refocus"), "iterefocus",
                NULL, 0,
                refocusit_help, PROCEDURE_NAME,
                GTK_STOCK_OK, GTK_RESPONSE_OK,
                GIMP_STOCK_RESET, RESPONSE_RESET, 
                _("Preview"), RESPONSE_PREVIEW,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
                NULL);

  g_signal_connect (dlg, "response", G_CALLBACK (dialog_response), NULL);
  g_signal_connect (dlg, "destroy", G_CALLBACK (destroy_callback), NULL);

  preview_parameters_init ();
  dialog_parameters_create ();
  dialog_parameters_init ();

  hbox = gtk_hbox_new (FALSE, 5);
  element = create_controls ();
  gtk_box_pack_start (GTK_BOX (hbox), element, TRUE, FALSE, 5);

  vbox = gtk_vbox_new (FALSE, 2);

  element = motion_angle_create ();
  gtk_box_pack_start (GTK_BOX (vbox), element, TRUE, FALSE, 5);

  element = preview_create ();
  gtk_box_pack_start (GTK_BOX (vbox), element, TRUE, FALSE, 5);

  gtk_widget_show (vbox);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 5);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, FALSE, 5);

  gtk_widget_show (GTK_WIDGET (hbox));
  gtk_widget_show (dlg);

  preview_update ();
  dialog_elements_update ();

  motion_angle_draw (TRUE);
  gtk_main ();

  dialog_elements_destroy ();

  return dialog_parameters.frun;
}

static void get_lambdas (gdouble* lambda, gdouble* lambda_min) {
  *lambda_min = 1.0 / exp (input_parameters.lambda_min / 4.0);
  *lambda = input_parameters.lambda / LAMBDA_MAX * 0.001 / *lambda_min;
}

static void event_loop () {
  while (gtk_events_pending()) gtk_main_iteration_do(TRUE);
}

static void progress_bar_init () {
  if (dialog_elements.progress) {
    gtk_progress_bar_update (GTK_PROGRESS_BAR (dialog_elements.progress), 0.0);
  } else {
    gimp_progress_init(_("Refocusing..."));
  }
}

static void progress_bar_update (gfloat fraction) {
  if (dialog_elements.progress) {
    gtk_progress_bar_update (GTK_PROGRESS_BAR (dialog_elements.progress), fraction);
  } else {
    gimp_progress_update (fraction);
  }

  event_loop ();
}

static void progress_bar_reset () {
  if (dialog_elements.progress) {
    gtk_progress_bar_update (GTK_PROGRESS_BAR (dialog_elements.progress), 0.0);
  } else {
    gimp_progress_update (0.0);
  }
}

static int compute (int iterations) {
  int i;
  gdouble lambda_min, lambda;
  gfloat step, final;
  gboolean is_adaptive, is_smooth, is_mirror;
  convmask_t defoc, gauss, motion, blur;

  event_loop ();

  get_lambdas (&lambda, &lambda_min);

  is_smooth = (lambda > 1e-8 && lambda_min < LAMBDAMIN_USABLE_MAX);
  is_adaptive = (input_parameters.adaptive_smooth && is_smooth);
  is_mirror = (input_parameters.boundary == BOUNDARY_MIRROR);

  /* PROGRESS BAR */
  step = 1.0;
  final = (gfloat)iterations;
  if (is_adaptive) {
    final *= 2;
  } else if (is_smooth) {
    final++;
  }
  if (image_parameters.rgb) {
    final *= 3.0;
  }

  progress_bar_init ();

  hopfield_data_load ();
  preview_update ();

  if (blur_create_defocus (&defoc, (double)input_parameters.radius) == NULL) goto compute_err0;
  if (blur_create_gauss (&gauss, (double)input_parameters.gauss) == NULL) goto compute_err1;
  if (blur_create_motion (&motion, (double)input_parameters.motion, (double)input_parameters.mot_angle) == NULL) goto compute_err2;
  if (convmask_convolve (&blur, &defoc, &gauss) == NULL)  goto compute_err3;
  if (convmask_convolve (&hopfield.blur, &blur, &motion) == NULL) goto compute_err4;
  convmask_destroy (&blur);
  convmask_destroy (&motion);
  convmask_destroy (&gauss);
  convmask_destroy (&defoc);
#if defined(NDEBUG)
  int x, y, r;
  printf("combine blur+motion+guass+defocus using convmask_convolve()");
  convmask_print(&hopfield.blur, "hopfield.blur");
#endif

  if (is_smooth) {
    if (blur_create_gauss (&hopfield.filter, 1.0) == NULL) goto compute_err5;
    lambda_set_mirror (&hopfield.lambdafldR, is_mirror);
    lambda_set_nl (&hopfield.lambdafldR, TRUE);
    if (lambda_create (&hopfield.lambdafldR, image_parameters.sel_width, image_parameters.sel_height, lambda_min, input_parameters.winsize, &hopfield.filter) == NULL) goto compute_err6;
#if defined(NDEBUG)
    x = image_parameters.sel_width;
    y = image_parameters.sel_height;
    r = hopfield.filter.radius;
    printf("is_smooth, lambda_create(), x=%d y=%d lambda_min=%g x=%d y=%d winsize=%d combined radius=%d\n", x, y, lambda_min, hopfield.lambdafldR.x, hopfield.lambdafldR.y, input_parameters.winsize, r);
    printf("hopfield.lambdafldR, hopfield.imageR\n");
    for (y = 0; y <= 8; y++) {
      for (x = 0; x <= 8; x++) {
        printf("|%d %d %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x]);
      }
      printf("\n");
    }
    convmask_print(&hopfield.filter, "hopfield.filter");
#endif
    if (image_parameters.rgb) {
      lambda_set_mirror (&hopfield.lambdafldG, is_mirror);
      lambda_set_mirror (&hopfield.lambdafldB, is_mirror);
      lambda_set_nl (&hopfield.lambdafldG, TRUE);
      lambda_set_nl (&hopfield.lambdafldB, TRUE);
      if (lambda_create (&hopfield.lambdafldG, image_parameters.sel_width, image_parameters.sel_height, lambda_min, input_parameters.winsize, &hopfield.filter) == NULL) goto compute_err7;
      if (lambda_create (&hopfield.lambdafldB, image_parameters.sel_width, image_parameters.sel_height, lambda_min, input_parameters.winsize, &hopfield.filter) == NULL) goto compute_err8;
    }
#if defined(NDEBUG)
    printf("..did smooth (before !is_adaptive)\n");
#endif

    if (!is_adaptive) {
      if (lambda_calculate (&hopfield.lambdafldR, &hopfield.imageR) == NULL) goto compute_err9;
      progress_bar_update(step++ / final);
#if defined(NDEBUG)
    x = hopfield.lambdafldR.x;
    y = hopfield.lambdafldR.y;
    printf("!is_adaptive, lambda_calculate(), x=%d y=%d, hopfield.lambdafldR.lambda[] hopfield.imageR[]\n", x, y);
    for (y = 0; y <= 8; y++) {
      for (x = 0; x <= 8; x++) {
        printf("|%d %d %f %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x], image_get(&hopfield.imageR,x,y) );
      }
      printf("\n");
    }
#endif
      if (image_parameters.rgb) {
        if (lambda_calculate (&hopfield.lambdafldG, &hopfield.imageG) == NULL) goto compute_err9;
        progress_bar_update (step++ / final);
        if (lambda_calculate (&hopfield.lambdafldB, &hopfield.imageB) == NULL) goto compute_err9;
        progress_bar_update (step++ / final);
      }
#if defined(NDEBUG)
      printf("..did !is_adaptive, lambda=%g\n", lambda);
#endif
    }
  }

  hopfield.hopfieldR.lambda = lambda;
  hopfield_set_mirror (&hopfield.hopfieldR, is_mirror);
  if (is_smooth) {
    if (hopfield_create (&hopfield.hopfieldR, &hopfield.blur, &hopfield.imageR, &hopfield.lambdafldR) == NULL) goto compute_err9;
  } else {
    if (hopfield_create (&hopfield.hopfieldR, &hopfield.blur, &hopfield.imageR, NULL) == NULL) goto compute_err9;
  }
#if defined(NDEBUG)
  x = hopfield.lambdafldR.x;
  y = hopfield.lambdafldR.y;
  printf("is_smooth, hopfield_create(), x=%d y=%d\n", x, y);
  printf("hopfield.lambdafldR.lamba[], hopfield.imageR[]\n");
  for (y = 0; y <= 8; y++) {
    for (x = 0; x <= 8; x++) {
      printf("|%d %d %f %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x], image_get(&hopfield.imageR,x,y) );
    }
    printf("\n");
  }
  printf("weights=");
  weights_print(&hopfield.hopfieldR.weights, "hopfield.blur");
  convmask_print(&hopfield.blur, "hopfield.blur");
#endif
  if (image_parameters.rgb) {
    hopfield.hopfieldG.lambda = lambda;
    hopfield.hopfieldB.lambda = lambda;
    hopfield_set_mirror (&hopfield.hopfieldG, is_mirror);
    hopfield_set_mirror (&hopfield.hopfieldB, is_mirror);
    if (is_smooth) {
      if (hopfield_create (&hopfield.hopfieldG, &hopfield.blur, &hopfield.imageG, &hopfield.lambdafldG) == NULL) goto compute_err10;
      if (hopfield_create (&hopfield.hopfieldB, &hopfield.blur, &hopfield.imageB, &hopfield.lambdafldB) == NULL) goto compute_err11;
    } else {
      if (hopfield_create (&hopfield.hopfieldG, &hopfield.blur, &hopfield.imageG, NULL) == NULL) goto compute_err10;
      if (hopfield_create (&hopfield.hopfieldB, &hopfield.blur, &hopfield.imageB, NULL) == NULL) goto compute_err11;
    }
  }
#if defined(NDEBUG)
  printf("..did lambda = %g, now do iterations=%d\n", lambda, iterations);
#endif

  for (i = 1; i <= iterations; i++) {
    if (is_adaptive) {
      if (lambda_calculate (&hopfield.lambdafldR, &hopfield.imageR) == NULL) goto compute_err12;

      progress_bar_update (step++ / final);
      if (dialog_parameters.finish) break;

      if (image_parameters.rgb) {
        if (lambda_calculate (&hopfield.lambdafldG, &hopfield.imageG) == NULL) goto compute_err12;

        progress_bar_update (step++ / final);
        if (dialog_parameters.finish) break;

        if (lambda_calculate (&hopfield.lambdafldB, &hopfield.imageB) == NULL) goto compute_err12;

        progress_bar_update (step++ / final);
        if (dialog_parameters.finish) break;
      }
    }
    hopfield_iteration (&hopfield.hopfieldR);

    progress_bar_update (step++ / final);
    if (dialog_parameters.finish) break;

    if (image_parameters.rgb) {
      hopfield_iteration (&hopfield.hopfieldG);

      progress_bar_update (step++ / final);
      if (dialog_parameters.finish) break;

      hopfield_iteration (&hopfield.hopfieldB);

      progress_bar_update (step++ / final);
      if (dialog_parameters.finish) break;
    }

    preview_update ();

    while (gtk_events_pending ()) gtk_main_iteration_do(TRUE);
    if (dialog_parameters.finish) break;
  }

  if (image_parameters.rgb) {
    hopfield_destroy (&hopfield.hopfieldB);
    hopfield_destroy (&hopfield.hopfieldG);
  }
  hopfield_destroy (&hopfield.hopfieldR);
  if (is_smooth) {
    if (image_parameters.rgb) {
      lambda_destroy (&hopfield.lambdafldB);
      lambda_destroy (&hopfield.lambdafldG);
    }
    lambda_destroy (&hopfield.lambdafldR);
    convmask_destroy (&hopfield.filter);
  }
  convmask_destroy(&hopfield.blur);

  if (!dialog_parameters.finish) {
    progress_bar_reset ();
  }
  return 1;

compute_err12:
  if (!image_parameters.rgb) goto compute_err10;
  hopfield_destroy (&hopfield.hopfieldB);
compute_err11:
  hopfield_destroy (&hopfield.hopfieldG);
compute_err10:
  hopfield_destroy (&hopfield.hopfieldR);
compute_err9:
  if (!image_parameters.rgb) goto compute_err7;
  if (&hopfield.lambdafldB) lambda_destroy (&hopfield.lambdafldB);
compute_err8:
  if (&hopfield.lambdafldG) lambda_destroy (&hopfield.lambdafldG);
compute_err7:
  if (&hopfield.lambdafldR) lambda_destroy (&hopfield.lambdafldR);
compute_err6:
    convmask_destroy (&hopfield.filter);
compute_err5:
  convmask_destroy (&hopfield.blur);
  return 0;

compute_err4:
  convmask_destroy (&blur);
compute_err3:
  convmask_destroy (&motion);
compute_err2:
  convmask_destroy (&gauss);
compute_err1:
  convmask_destroy (&defoc);
compute_err0:
  return 0;
}

static void
run (const gchar *name, gint nparams, const GimpParam *param,
     gint *nreturn_vals, GimpParam **return_vals) {
  GimpRunMode       run_mode; /* Current run mode */
  GimpPDBStatusType status;   /* Return status */
  GimpParam        *values;   /* Return values */
  gchar            *title;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);
#endif

  /* Set mandatory return values */
  *nreturn_vals = 1;
  values = g_new (GimpParam, 1);
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
  *return_vals  = values;
  status = GIMP_PDB_SUCCESS;

  /* Initialize parameter data... */
  input_parameters_init ();
  if (image_parameters_init (param, values)) return;
  if (hopfield_data_init ()) return;
  hopfield_data_load ();

  /* See how we will run */
  run_mode = param[0].data.d_int32;
  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    /*INIT_I18N_UI();*/
    title = g_strdup_printf (_("Iterative Refocus - %s"), PACKAGE_VERSION);
    gimp_ui_init (title, TRUE);
    input_parameters_load ();
    if (dialog ()) {
      input_parameters_save ();
    }
    gimp_displays_flush ();
    break;

  case GIMP_RUN_NONINTERACTIVE:
    /*INIT_I18N();*/
    if (nparams != 11) status = GIMP_PDB_CALLING_ERROR;
    else {
      input_parameters_fetch_params (param);
      compute (input_parameters.iterations);
    }
    break;

  case GIMP_RUN_WITH_LAST_VALS:
    /*INIT_I18N();*/
    input_parameters_load ();
    compute (input_parameters.iterations);
    gimp_displays_flush ();
    break;

  default:
    status = GIMP_PDB_CALLING_ERROR;
    break;
  };

  /* Detach from the drawable... */
  hopfield_data_destroy ();
  image_parameters_destroy ();
  input_parameters_destroy ();

  gegl_exit();

  values[0].data.d_status = status;
}

static void refocusit_help (const gchar *help_id, gpointer help_data) {
  gimp_message(_(LONG_DESCRIPTION));
}
