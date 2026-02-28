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

#ifdef HAVE_GETTEXT
//#include "gettext.h"
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define d_(String) String
#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif
#else
/* No i18n for now */
#define _(String) String
#define gettext_noop(String) String
#define d_(String) String
#define N_(String) String
#endif

#define PREVIEW_SIZE		128
#define MOTION_ANGLE_DRA_SIZE	80
#define MOTION_ANGLE_DRA_MIDDLE	(MOTION_ANGLE_DRA_SIZE / 2)
#define MOTION_ANGLE_BUTTON	1
#define LAMBDAMIN_MAX		100.0
#define LAMBDAMIN_USABLE_MAX	0.999
#define LAMBDA_MAX		10000.0

#define RESPONSE_PREVIEW	1
#define RESPONSE_RESET		2

/* DATA STRUCTURES */

enum {
  BOUNDARY_MIRROR = 0,
  BOUNDARY_PERIODICAL,
  BOUNDARY_LAST
};

typedef struct _RefocusIt RefocusIt;
typedef struct _RefocusItClass RefocusItClass;

struct _RefocusIt {
  GimpPlugIn parent_instance;
};

struct _RefocusItClass {
  GimpPlugInClass parent_class;
};

typedef struct {
  char          *name;
  GtkWidget     *menu_item;
} SListbox;

typedef void (*FListboxHandler)(GtkWidget*, guint);

typedef struct {
  GtkWidget     *preview;
  guint          x;
  guint          y;
  guint          width;
  guint          height;
  guint          size;
  guchar        *data;
  gdouble       *linear;
  const Babl    *rgb8;
} SPreview;

typedef struct {
  gdouble        radius;
  gdouble        gauss;
  gdouble        motion;
  gdouble        mot_angle;
  gdouble        lambda;
  gdouble        lambda_min;
  guint          winsize;
  guint          iterations;
  guint          boundary;
  guint          adaptive_smooth;
  guint          prev_iter;
} SInputParameters;

typedef struct {
  guint          sel_width;
  guint          sel_height;
  gint           img_bpp;
  guint          size;
  gboolean       gray;
  gboolean       rgb;
  GimpDrawable  *drawable;
  GeglBuffer    *srcBuf;
  GeglBuffer    *destBuf;
  gdouble       *srcImg;
  guchar        *destImg;
  const Babl    *format;
  const Babl    *linear;
  gint           xImg;
  gint           yImg;
  gint           bppImg;
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
  GtkWidget     *progress;
  GtkWidget     *motion_angle_dra;
  GtkWidget     *adaptive;
  GtkWidget     *area_smooth;
  GtkWidget     *boundary;
  GtkWidget     *dialog;
} SDialogElements;

typedef struct {
  image_t        imageR;
  image_t        imageG;
  image_t        imageB;
  hopfield_t     hopfieldR;
  hopfield_t     hopfieldG;
  hopfield_t     hopfieldB;
  convmask_t     blur;
  convmask_t     filter;
  lambda_t       lambdafldR;
  lambda_t       lambdafldG;
  lambda_t       lambdafldB;
} SHopfield;

/* STATIC DATA */

static SDialogElements    dialog_elements;
static SDialogParameters  dialog_parameters;
static SInputParameters   input_parameters;
static SImageParameters   image_parameters;
static SPreview           preview;
static SHopfield          hopfield;
static SListbox           boundary_listbox[BOUNDARY_LAST + 1];

/* Declare local functions. */
#define REFOCUSIT_TYPE  (refocusit_get_type())
#define REFOCUSIT (obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), REFOCUSIT_TYPE, RefocusIt))

/* CONSTANTS */

static const char PLUG_IN_PROC[] = "plug-in-" PLUGIN_NAME;
//static const char PLUG_IN_ROLE[] = "gimp-" PLUGIN_NAME;
static const char PLUG_IN_BINARY[] =  PLUGIN_NAME;
static const char PLUG_IN_MENU_LOCATION[] = "<Image>/Filters/Enhance";
static const char PLUG_IN_MENU_LABEL[] = d_("Iterative Refocus...");
static const char PLUG_IN_SHORT_DESC[] = d_(
  "This plug-in iteratively refocuses a defocused image.");
static const char PLUG_IN_LONG_DESC[] = d_(
  "Refocus-it can refocus images acquired by a defocused camera "
  "blurred by gaussian or motion blur or combination of these.\n\n"
  "Nice features include adaptive/static area smoothing to reduce "
  "'ringing' introduced by image edges and effects introduced by "
  "noise. Mirror and periodical boundary conditions are available. "
  "Preview helps you select the best parameters.\n\n"
  "NOT nice features are memory and CPU requirements.\n\n"
  "Refocus-it is based on finding the minimum error using the "
  "Hopfield neural network.");
static const char BOUNDARY_TEXT_MIRROR[] = d_("mirror boundary");
static const char BOUNDARY__TEXT_PERIODICAL[] = d_("periodical boundary");

/* FORWARD DECLARATIONS */

GType                   refocusit_get_type         (void) G_GNUC_CONST;

static GList          * refocusit_query_procedures (GimpPlugIn          *plug_in);
static GimpProcedure  * refocusit_create_procedure (GimpPlugIn          *plug_in,
                                                    const gchar         *name);

static GimpValueArray * refocusit_run              (GimpProcedure       *procedure,
                                                    GimpRunMode          run_mode,
                                                    GimpImage           *image,
                                                    GimpDrawable       **drawables,
                                                    GimpProcedureConfig *proc_config,
                                                    gpointer             run_data);

static void             refocusit_help             (const gchar         *help_id,
                                                    gpointer             help_data);

static void dialog_parameters_create ();
static void dialog_parameters_init ();
static void dialog_elements_update ();
static void dialog_elements_destroy ();
static void dialog_response(GtkWidget *widget, gint response_id, gpointer data);

static void input_parameters_reset ();
//static void input_parameters_load ();
//static void input_parameters_save ();
static void input_parameters_fetch_params (GimpProcedureConfig *proc_config);
static void input_parameters_fetch_dlg();
static int  image_parameters_init (GimpDrawable *drawable);
static void image_parameters_destroy ();
static int  hopfield_data_init ();
static void hopfield_data_destroy ();
static void hopfield_data_load ();
static void hopfield_data_save ();
static void preview_parameters_init ();
static void preview_fetch_hopfield ();
static void preview_update ();
static void get_lambdas (gdouble *lambda, gdouble *lambda_min);
static int compute (int iterations);
static void motion_angle_draw (gboolean complete_redraw);
static void motion_angle_xy_calculate (gdouble x, gdouble y);

G_DEFINE_TYPE (RefocusIt, refocusit, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (REFOCUSIT_TYPE)
//DEFINE_STD_SET_I18N; ***don't use, need 3rd-party locale external to gimp3

static void
refocusit_class_init (RefocusItClass *klass) {
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = refocusit_query_procedures;
  plug_in_class->create_procedure = refocusit_create_procedure;
  //plug_in_class->set_i18n       = STD_SET_I18N;
}

static void
refocusit_init (RefocusIt *refocusit) {
}

static GList *
refocusit_query_procedures (GimpPlugIn *plug_in) {
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

/* CALLBACKS */

static void no_smooth_callback (GtkWidget *widget, gpointer data) {
  if (dialog_elements.area_smooth) {
    if (gtk_adjustment_get_value (dialog_parameters.lambda) < 1e-6) {
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
  motion_angle_draw (FALSE);
}

static gboolean motion_vector_expose_callback (GtkWidget *widget, GdkEvent *gdkev, gpointer data) {
  motion_angle_draw (TRUE);
  return FALSE;
}

static gboolean motion_vector_mouse_move_callback (GtkWidget *widget, GdkEvent *gdkev, gpointer data) {
  motion_angle_xy_calculate (gdkev->motion.x, gdkev->motion.y);
  return FALSE;
}

static gboolean motion_vector_mouse_release_callback (GtkWidget *widget, GdkEvent *gdkev, gpointer data) {
  if (gdkev->button.button == MOTION_ANGLE_BUTTON) {
    g_signal_handlers_disconnect_by_func (G_OBJECT (widget), G_CALLBACK (motion_vector_mouse_release_callback), NULL);
    g_signal_handlers_disconnect_by_func (G_OBJECT (widget), G_CALLBACK (motion_vector_mouse_move_callback), NULL);
  }
  return FALSE;
}

static gboolean motion_vector_mouse_press_callback (GtkWidget *widget, GdkEvent *gdkev, gpointer data) {
  if (gdkev->button.button == MOTION_ANGLE_BUTTON) {
    motion_angle_xy_calculate (gdkev->button.x, gdkev->button.y);
    g_signal_connect (G_OBJECT (widget), "button-release-event", G_CALLBACK (motion_vector_mouse_release_callback), NULL);
    g_signal_connect (G_OBJECT (widget), "motion-notify-event", G_CALLBACK (motion_vector_mouse_move_callback), NULL);
  }
  return FALSE;
}

static void boundary_callback (GtkWidget *menu_item, guint index) {
//static void boundary_callback (GtkWidget* menu_item, gpointer *index) {
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
  input_parameters_reset ();
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
  preview.x = (guint)(gtk_adjustment_get_value (dialog_parameters.hscroll));
  preview.y = (guint)(gtk_adjustment_get_value (dialog_parameters.vscroll));
  preview_update ();
}

/* FUNCTIONS */

static GimpProcedure *
refocusit_create_procedure (GimpPlugIn *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;
  gchar *longdesc;

  if (!strcmp (name, PLUG_IN_PROC)) {

    procedure = gimp_image_procedure_new (plug_in, name,
                                          GIMP_PDB_PROC_TYPE_PLUGIN,
                                          refocusit_run, NULL, NULL);

    gimp_procedure_set_image_types (procedure, "RGB*,GRAY*");
    gimp_procedure_set_sensitivity_mask (procedure,
                                         GIMP_PROCEDURE_SENSITIVE_DRAWABLE);
#ifdef HAVE_GETTEXT
    /* Initialize i18n support */
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
    textdomain (GETTEXT_PACKAGE);
#endif

    gimp_procedure_set_menu_label (procedure, _(PLUG_IN_MENU_LABEL));
    gimp_procedure_add_menu_path (procedure, PLUG_IN_MENU_LOCATION);
    longdesc = g_strdup_printf (_(PLUG_IN_LONG_DESC));
    gimp_procedure_set_documentation (procedure,
    /* menu entry tooltip blurb   */  _(PLUG_IN_SHORT_DESC),
    /* help for script developers */  longdesc,
    /* help ID                    */  PLUG_IN_PROC);
    g_free (longdesc);
    gimp_procedure_set_attribution (procedure,
    /* author(s) original, GIMP3 */ "Lukas Kunc (2003), Jose Da Silva (2026)",
    /* copyright license         */ "GPL3+",
    /* date for the latest build */ "2026");

    gimp_procedure_add_double_argument (procedure, "radius",
                                        _("_Radius"), _("Blur radius (default = 6.0)"),
                                        0.0, 32.0, 6.0,
                                        G_PARAM_READWRITE);
    gimp_procedure_add_double_argument (procedure, "gauss",
                                        _("_Gauss"), _("Gaussian blur variance (default = 0.0)"),
                                        0, 32.0, 0.0,
                                        G_PARAM_READWRITE);
    gimp_procedure_add_double_argument (procedure, "motion",
                                        _("Motion _Size"), _("Motion size (default = 0.0)"),
                                        0.0, 32.0, 0.0,
                                        G_PARAM_READWRITE);
    gimp_procedure_add_double_argument (procedure, "mot_angle",
                                        _("Motion _Angle"), _("Motion angle (default = 0.0)"),
                                        0.0, 359.9, 0.0,
                                        G_PARAM_READWRITE);
    gimp_procedure_add_double_argument (procedure, "lambda",
                                        _("_Lambda Noise"), _("Noise reduction (default = 100.0)"),
                                        0.0, (gdouble)(LAMBDA_MAX), 100.0,
                                        G_PARAM_READWRITE);
    gimp_procedure_add_int_argument (procedure, "boundary",
                                     _("_Boundary"), _("Boundary conditions (default = mirror / 0)"),
                                     0, 2, 0, G_PARAM_READWRITE);
    gimp_procedure_add_double_argument (procedure, "lambda_min",
                                        _("Lambda _Min"), _("Area smoothnes (default = 30.0)"),
                                        0.0, (gdouble)(LAMBDAMIN_MAX), 30.0,
                                        G_PARAM_READWRITE);
    gimp_procedure_add_int_argument (procedure, "adaptive_smooth",
                                     _("Adaptive Smoothing"), _("Adaptive smoothing (default = TRUE)"),
                                     0, 1, 1, G_PARAM_READWRITE);
    gimp_procedure_add_int_argument (procedure, "winsize",
                                     _("_Window Size"),  _("Smooth area size (default = 3)"),
                                     1, 16, 1, G_PARAM_READWRITE);
    gimp_procedure_add_int_argument (procedure, "iterations",
                                     _("_Iterations"),  _("Number of iterations (default = 100)"),
                                     1, 200, 100, G_PARAM_READWRITE);
    gimp_procedure_add_int_argument (procedure, "prev_iter",
                                     _("_Preview Iterations"),
                                     _("Number of iterations for preview (default = 10)"),
                                     1, 20, 10, G_PARAM_READWRITE);
  }

  return procedure;
}

static void input_parameters_reset () {
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

//static void input_parameters_load () {
//  //gimp_get_data (PACKAGE_NAME, &input_parameters);
//  gimp_procedural_db_get_data (PACKAGE_NAME, &input_parameters);
//}

//static void input_parameters_save () {
//  gimp_procedural_db_set_data (PACKAGE_NAME, &input_parameters, sizeof (input_parameters));
//}

static void input_parameters_fetch_params (GimpProcedureConfig *proc_config) {
  if (proc_config) {
    g_object_get (proc_config,
                  "radius",          &input_parameters.radius,
                  "gauss",           &input_parameters.gauss,
                  "motion",          &input_parameters.motion,
                  "mot_angle",       &input_parameters.mot_angle,
                  "lambda",          &input_parameters.lambda,
    /* int */     "boundary",        &input_parameters.boundary,
                  "lambda_min",      &input_parameters.lambda_min,
    /* int */     "adaptive_smooth", &input_parameters.adaptive_smooth,
    /* int */     "winsize",         &input_parameters.winsize,
    /* int */     "iterations",      &input_parameters.iterations,
    /* int */     "prev_iter",       &input_parameters.prev_iter,
                  NULL);
  } else {
    input_parameters_reset ();
  }
}

static void input_parameters_fetch_dlg () {
  input_parameters.radius     = gtk_adjustment_get_value (dialog_parameters.radius);
  input_parameters.gauss      = gtk_adjustment_get_value (dialog_parameters.gauss);
  input_parameters.motion     = gtk_adjustment_get_value (dialog_parameters.motion);
  input_parameters.mot_angle  = gtk_adjustment_get_value (dialog_parameters.mot_angle);
  input_parameters.lambda     = gtk_adjustment_get_value (dialog_parameters.lambda);
  /* no action for boundary - updated automatically */
  input_parameters.lambda_min = gtk_adjustment_get_value (dialog_parameters.lambda_min);
  input_parameters.winsize    = (guint)(gtk_adjustment_get_value (dialog_parameters.winsize));
  input_parameters.iterations = (guint)(gtk_adjustment_get_value (dialog_parameters.iterations));
  input_parameters.prev_iter  = (guint)(gtk_adjustment_get_value (dialog_parameters.prev_iter));
  input_parameters.adaptive_smooth = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog_elements.adaptive));
}

static void dialog_parameters_init () {
  gtk_adjustment_set_value (dialog_parameters.radius,     input_parameters.radius);
  gtk_adjustment_set_value (dialog_parameters.gauss,      input_parameters.gauss);
  gtk_adjustment_set_value (dialog_parameters.motion,     input_parameters.motion);
  gtk_adjustment_set_value (dialog_parameters.mot_angle,  input_parameters.mot_angle);
  gtk_adjustment_set_value (dialog_parameters.lambda,     input_parameters.lambda);
  gtk_adjustment_set_value (dialog_parameters.lambda_min, input_parameters.lambda_min);
  gtk_adjustment_set_value (dialog_parameters.winsize,    input_parameters.winsize);
  gtk_adjustment_set_value (dialog_parameters.iterations, input_parameters.iterations);
  gtk_adjustment_set_value (dialog_parameters.prev_iter,  input_parameters.prev_iter);
  dialog_parameters.area_smooth_enabled = TRUE;
}

static void dialog_parameters_create () {
  dialog_parameters.frun = FALSE;
  dialog_parameters.finish = FALSE;

  dialog_parameters.radius     = GTK_ADJUSTMENT (gtk_adjustment_new (input_parameters.radius, 0.0, 32.0, 0.01, 1.0, 0.0));
  dialog_parameters.gauss      = GTK_ADJUSTMENT (gtk_adjustment_new (input_parameters.gauss, 0.0, 32.0, 0.01, 1.0, 0.0));
  dialog_parameters.motion     = GTK_ADJUSTMENT (gtk_adjustment_new (input_parameters.motion, 0.0, 32.0, 0.01, 1.0, 0.0));
  dialog_parameters.mot_angle  = GTK_ADJUSTMENT (gtk_adjustment_new (input_parameters.motion, 0.0, 360.0, 0.01, 1.0, 0.0));
  dialog_parameters.lambda     = GTK_ADJUSTMENT (gtk_adjustment_new (input_parameters.lambda, 0.0, (gdouble)LAMBDA_MAX, 0.1, 1.0, 0.0));
  dialog_parameters.lambda_min = GTK_ADJUSTMENT (gtk_adjustment_new (input_parameters.lambda_min, 0.0, (gdouble)LAMBDAMIN_MAX, 0.1, 1.0, 0.0));
  dialog_parameters.winsize    = GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble)(input_parameters.winsize), 1.0, 16.0, 1.0, 2.0, 0.0));
  dialog_parameters.iterations = GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble)(input_parameters.iterations), 1.0, 200.0, 1.0, 10.0, 0.0));
  dialog_parameters.prev_iter  = GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble)(input_parameters.prev_iter), 1.0, 20.0, 1.0, 1.0, 0.0));
  dialog_parameters.hscroll    = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, (gdouble)(image_parameters.sel_width) - 1.0, 1.0, (gdouble)(preview.width), (gdouble)(preview.width)));
  dialog_parameters.vscroll    = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, (gdouble)(image_parameters.sel_height) - 1.0, 1.0, (gdouble)(preview.height), (gdouble)(preview.height)));

  g_signal_connect (G_OBJECT (dialog_parameters.mot_angle), "value_changed", G_CALLBACK (motion_vector_change_callback), NULL);
  g_signal_connect (G_OBJECT (dialog_parameters.lambda), "value_changed", G_CALLBACK (no_smooth_callback), NULL);
  g_signal_connect (G_OBJECT (dialog_parameters.hscroll), "value_changed", G_CALLBACK (preview_scroll_callback), NULL);
  g_signal_connect (G_OBJECT (dialog_parameters.vscroll), "value_changed", G_CALLBACK (preview_scroll_callback), NULL);

  boundary_listbox[BOUNDARY_MIRROR].name = _(BOUNDARY_TEXT_MIRROR);
  boundary_listbox[BOUNDARY_PERIODICAL].name = _(BOUNDARY__TEXT_PERIODICAL);
  boundary_listbox[BOUNDARY_LAST].name = NULL;
}

static void dialog_elements_update () {
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog_elements.adaptive), input_parameters.adaptive_smooth);
  //gtk_option_menu_set_history (GTK_OPTION_MENU (dialog_elements.boundary), input_parameters.boundary);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog_elements.boundary), input_parameters.boundary);
  if (dialog_elements.area_smooth && gtk_adjustment_get_value (dialog_parameters.lambda) < 1e-6) {
    gtk_widget_set_sensitive (GTK_WIDGET (dialog_elements.area_smooth), FALSE);
    dialog_parameters.area_smooth_enabled = FALSE;
  }
  motion_angle_draw (FALSE);
}

static void dialog_elements_destroy () {
  gtk_widget_destroy (dialog_elements.progress);
  gtk_widget_destroy (dialog_elements.adaptive);
  gtk_widget_destroy (dialog_elements.area_smooth);
  gtk_widget_destroy (dialog_elements.boundary);
  gtk_widget_destroy (dialog_elements.dialog);
  dialog_elements.progress    = NULL;
  dialog_elements.adaptive    = NULL;
  dialog_elements.area_smooth = NULL;
  dialog_elements.boundary    = NULL;
  dialog_elements.dialog      = NULL;

  g_free (boundary_listbox[BOUNDARY_MIRROR].name);
  g_free (boundary_listbox[BOUNDARY_PERIODICAL].name);
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

static int image_parameters_init (GimpDrawable *drawable) {
  image_parameters.drawable   = drawable;
  image_parameters.format     = gimp_drawable_get_format (drawable);
  image_parameters.rgb        = gimp_drawable_is_rgb (drawable);
  image_parameters.gray       = gimp_drawable_is_gray (drawable);
  if (!(image_parameters.rgb || image_parameters.gray))
    return -1;

  image_parameters.sel_width  = gimp_drawable_get_width (drawable);
  image_parameters.sel_height = gimp_drawable_get_height (drawable);
  image_parameters.img_bpp    = gimp_drawable_get_bpp (drawable);
  image_parameters.size       = image_parameters.sel_width * image_parameters.sel_height;

  preview.data = NULL;
  preview.linear = NULL;
  return 0;
}

static void image_parameters_destroy () {
  //gimp_drawable_detach (image_parameters.drawable);
  if (preview.data)   g_free(preview.data);
  if (preview.linear) g_free(preview.linear);
}

static void preview_parameters_init () {
  const char *str, *ret;

#if defined(NDEBUG)
    printf ("preview_parameters_init() - starting!\n");
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
    printf ("preview_parameters_init() - found <%s> and will output <%s>\n", str, ret);
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

static int hopfield_data_init () {
  gint xImg, yImg, pixelCount, bppImg;

  gegl_init (NULL, NULL);

  image_parameters.bppImg = bppImg = gimp_drawable_get_bpp (image_parameters.drawable);

  /* Load 'linear_double RGB' or 'linear_double Gray' into srcImg */
  image_parameters.xImg = xImg = gimp_drawable_get_width (image_parameters.drawable);
  image_parameters.yImg = yImg = gimp_drawable_get_height (image_parameters.drawable);
  pixelCount = xImg * yImg;
  if (!(image_parameters.srcImg = g_new (gdouble, (size_t)(pixelCount * (image_parameters.rgb ? 3:1)))))
    goto hopfield_data_init_err0;
  if (!(image_parameters.destImg = g_new (guchar, (size_t)(pixelCount * bppImg))))
    goto hopfield_data_init_err1;

  if (!(image_parameters.srcBuf = gimp_drawable_get_buffer (image_parameters.drawable))) {
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
  printf ("hopfield_data_init()\nDrawable image format <%s>, bytes per pixel=%d, xImg=%d yImg=%d\n",
         babl_get_name (image_parameters.format), image_parameters.bppImg, xImg, yImg);
  for (int i = 0; i <40; i++) {
    printf ("|%d-%d-%f",i,image_parameters.destImg[i],image_parameters.srcImg[i]);
  }
  printf ("\nBuffer srcImg format <%s>\n", babl_get_name (image_parameters.linear));
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
  printf ("Error, hopfield_data_init() - out of memory!\n");
#endif
  return -1;
}

static void hopfield_data_destroy () {
  if (image_parameters.rgb) {
    image_destroy (&hopfield.imageB);
    image_destroy (&hopfield.imageG);
  }
  image_destroy (&hopfield.imageR);

  g_free (image_parameters.destImg);
  g_free (image_parameters.srcImg);
}

static void hopfield_data_save () {
  gdouble *ptr;
  gint     x, y, xImg, yImg;

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
  printf ("hopfield_data_save() - converted srcImg back to drawable format!\n");
  printf ("Drawable image format <%s>, bytes per pixel=%d, xImg=%d yImg=%d\n",
         babl_get_name (image_parameters.format), image_parameters.bppImg, xImg, yImg);
  for (int i = 0; i <40; i++) {
    printf ("|%d-%f-%d",i,image_parameters.srcImg[i],image_parameters.destImg[i]);
  }
  printf ("\nBuffer srcImg format <%s>\n", babl_get_name (image_parameters.linear));
#endif

  /* merge the shadow, update the drawable */
  if (!(image_parameters.destBuf = gimp_drawable_get_shadow_buffer (image_parameters.drawable)))
    goto hopfield_data_save_err0;
  gegl_buffer_set (image_parameters.destBuf, GEGL_RECTANGLE(0, 0, xImg, yImg), 0, \
                   image_parameters.format, image_parameters.destImg, GEGL_AUTO_ROWSTRIDE);
    gegl_buffer_flush (image_parameters.destBuf);
    gimp_drawable_merge_shadow (image_parameters.drawable, TRUE);
#if defined(NDEBUG)
  printf ("hopfield_data_save() - shadow merged!\n");
#endif
  gimp_drawable_update (image_parameters.drawable, 0, 0, xImg, yImg);
  g_object_unref (image_parameters.destBuf);
  return;

hopfield_data_save_err0:
#if defined(NDEBUG)
  printf ("Error, hopfield_data_save() - out of memory!\n");
#endif
  return;
}

static void hopfield_data_load () {
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

static void preview_fetch_hopfield () {
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

static void preview_update () {
  GdkPixbuf *pixbuf;

  preview_fetch_hopfield ();

  pixbuf = gdk_pixbuf_new_from_data (preview.data, GDK_COLORSPACE_RGB, FALSE, 8,
                                     preview.width, preview.height, preview.width * 3,
                                     NULL, NULL);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview.preview), pixbuf);
  g_object_unref (pixbuf);

  gtk_widget_queue_draw (preview.preview);
  gdk_display_flush (gdk_display_get_default ());
}

/* GUI ELEMENTS */

static GtkWidget *scaler_new (GtkAdjustment *adj, gfloat climb_rate, guint digits) {
  GtkWidget *box;
  GtkWidget *element;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  /* hscale */
  element = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adj);
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

static GtkWidget *listbox_new (SListbox *listdef, FListboxHandler handler, guint active) {
  GtkWidget *element;
  GtkWidget *listbox;
  GtkWidget *menu;
  GtkWidget *menu_items;
  guint      item;
  GtkWidget *combo_box;

  combo_box = gtk_combo_box_text_new ();
  item = 0;
  while (listdef->name) {
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_box), listdef->name);

    g_signal_connect (G_OBJECT (combo_box), "changed", G_CALLBACK (handler), GUINT_TO_POINTER (item));
    gtk_widget_show (GTK_WIDGET (combo_box));
    listdef->menu_item = element;
    listdef++; item++;
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
  return combo_box;

//  listbox = gtk_option_menu_new ();
//  menu = gtk_menu_new ();
//  item = 0;
//  while (listdef->name) {
//    element = gtk_menu_item_new_with_label (listdef->name);
//    gtk_menu_shell_append (GTK_MENU_SHELL (menu), element);
//    g_signal_connect (G_OBJECT (element), "activate", G_CALLBACK (handler), GUINT_TO_POINTER (item));
//    gtk_widget_show (element);
//    listdef->menu_item = element;
//    listdef++; item++;
//  }
//
//  gtk_option_menu_set_menu (GTK_OPTION_MENU (listbox), menu);
//  gtk_option_menu_set_history (GTK_OPTION_MENU (listbox), active);
//  return listbox;
}

static GtkWidget *create_degradation_params () {
  GtkWidget *frame;
  GtkWidget *grid;
  GtkWidget *element;

  frame = gtk_frame_new (_("Degradation"));

  grid = gtk_grid_new ();

  /* blur radius */
  element = gtk_label_new (_("Radius:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 0, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.radius, 0.01, 2);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 0, 1, 1);
  gtk_widget_show (element);

  /* gaussian blur */
  element = gtk_label_new (_("Gauss:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 1, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.gauss, 0.01, 2);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 1, 1, 1);
  gtk_widget_show (element);

  /* motion blur */
  element = gtk_label_new (_("Motion size:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 2, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.motion, 0.01, 2);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 2, 1, 1);
  gtk_widget_show (element);

  /* motion angle */
  element = gtk_label_new (_("Motion angle:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 3, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.mot_angle, 0.01, 2);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 3, 1, 1);
  gtk_widget_show (element);

  /* noise reduction */
  element = gtk_label_new (_("Noise:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 4, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.lambda, 0.1, 1);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 4, 1, 1);
  gtk_widget_show (element);

  /* iterations */
  element = gtk_label_new (_("Iterations:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 5, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.iterations, 1, 0);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 5, 1, 1);
  gtk_widget_show (element);

  /* boundary */
  element = gtk_label_new (_("Boundary:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 6, 1, 1);
  gtk_widget_show (element);

  element = dialog_elements.boundary = listbox_new (boundary_listbox, boundary_callback, input_parameters.boundary);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 6, 1, 1);
  gtk_widget_show (element);

  gtk_container_set_border_width (GTK_CONTAINER (grid), 5);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 5);
  gtk_widget_show (grid);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (frame);
  return frame;
}

static GtkWidget *create_area_params () {
  GtkWidget *frame;
  GtkWidget *grid;
  GtkWidget *element;

  frame = gtk_frame_new (_("Area smoothing"));

  grid = gtk_grid_new ();

  element = gtk_label_new (_("Smoothness:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 0, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.lambda_min, 1.0, 1);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 0, 1, 1);
  gtk_widget_show (element);

  element = gtk_label_new (_("Area size:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 1, 1, 1);
  gtk_widget_show (element);

  element = scaler_new (dialog_parameters.winsize, 0.0, 0);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 1, 1, 1);
  gtk_widget_show (element);

  element = gtk_label_new (_("Adaptive smoothing:"));
  gtk_grid_attach (GTK_GRID (grid), element, 0, 2, 1, 1);
  gtk_widget_show (element);

  element = dialog_elements.adaptive = gtk_check_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (element), input_parameters.adaptive_smooth);
  gtk_grid_attach (GTK_GRID (grid), element, 1, 2, 1, 1);
  gtk_widget_show (element);

  gtk_container_set_border_width (GTK_CONTAINER (grid), 5);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 5);
  gtk_widget_show (grid);

  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (frame);
  return frame;
}

static GtkWidget *create_controls () {
  GtkWidget *vbox;
  GtkWidget *element;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

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
/*    if (complete_redraw) {
      gdk_draw_arc (dialog_elements.motion_angle_dra->window, dialog_elements.motion_angle_dra->style->black_gc, TRUE,
                    0, 0, MOTION_ANGLE_DRA_SIZE, MOTION_ANGLE_DRA_SIZE, 0, 360*64);
    }
    a = gtk_adjustment_get_value (dialog_parameters.mot_angle) * M_PI / 180.0;
    x = y = MOTION_ANGLE_DRA_MIDDLE - 1;
    x *= cos(a);
    y *= sin(a);
    gdk_draw_line (dialog_elements.motion_angle_dra->window, dialog_elements.motion_angle_dra->style->black_gc,
                   MOTION_ANGLE_DRA_MIDDLE, MOTION_ANGLE_DRA_MIDDLE, ox, oy);

    ox = MOTION_ANGLE_DRA_MIDDLE + (gint)(x+0.5);
    oy = MOTION_ANGLE_DRA_MIDDLE - (gint)(y+0.5);

    gdk_draw_line (dialog_elements.motion_angle_dra->window, dialog_elements.motion_angle_dra->style->white_gc,
                   MOTION_ANGLE_DRA_MIDDLE, MOTION_ANGLE_DRA_MIDDLE, ox, oy);
    gdk_display_flush (gdk_display_get_default ());
*/  }
}

static GtkWidget *motion_angle_create () {
  GtkWidget *frame;
  GtkWidget *box;
  GtkWidget *drawing_area; /* dialog_elements.motion_angle_dra */

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  frame = gtk_frame_new (_("Motion direction"));
  drawing_area = dialog_elements.motion_angle_dra = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, MOTION_ANGLE_DRA_SIZE, MOTION_ANGLE_DRA_SIZE);
  gtk_widget_show (drawing_area);

  gtk_box_pack_start (GTK_BOX (box), drawing_area, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_widget_show (box);

  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (frame);

  gtk_widget_add_events (drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
  g_signal_connect (G_OBJECT (drawing_area), "expose-event", G_CALLBACK (motion_vector_expose_callback), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "button-press-event", G_CALLBACK (motion_vector_mouse_press_callback), NULL);

  return frame;
}

static void motion_angle_xy_calculate (gdouble x, gdouble y) {
  gdouble r, a;

  x -= MOTION_ANGLE_DRA_MIDDLE;
  y = MOTION_ANGLE_DRA_MIDDLE - y;
  r = sqrt(x*x + y*y);
  if (r < 1e-4)
    a = 0.0;
  else {
    a = acos(x/r) * 180.0 / M_PI;
    if (y < 0.0) a = 360.0 - a;
  }
  gtk_adjustment_set_value (dialog_parameters.mot_angle, a);
}

static GtkWidget *preview_create () {
  GtkWidget *frame;
  GtkWidget *vbox, *hbox;
  GtkWidget *element;
  GtkWidget *image;
  GtkWidget *scrollbar;
  GtkWidget *grid;
  GdkPixbuf *pixbuf;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 0);

  gtk_grid_set_row_spacing (GTK_GRID (grid), 0);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 0);

  /* preview */
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, preview.width, preview.height);
  element = preview.preview = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  gtk_grid_attach (GTK_GRID (grid), element, 0, 0, 1, 1);
  gtk_widget_show (element);

  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (dialog_parameters.hscroll));
//unnecessary in gtk2 gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_ALWAYS);
  gtk_grid_attach (GTK_GRID (grid), scrollbar, 0, 1, 1, 1);
  gtk_widget_show (scrollbar);

  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT (dialog_parameters.vscroll));
//unnecessary in gtk2 gtk_range_set_update_policy (GTK_RANGE (scrollbar), GTK_UPDATE_ALWAYS);
  gtk_grid_attach (GTK_GRID (grid), scrollbar, 1, 0, 1, 1);
  gtk_widget_show (scrollbar);

  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* iterations */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  element = gtk_label_new (_("Iterations:"));
//  gtk_misc_set_alignment (GTK_MISC (element), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), element, FALSE, FALSE, 0);
  gtk_widget_show (element);

  element = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, dialog_parameters.prev_iter);
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

static gboolean dialog () {
  GtkWidget *element;
  GtkWidget *hbox;
  GtkWidget *dialog;
  GtkWidget *vbox;
  gchar     *title;

  title = g_strdup_printf (_("Iterative Refocus"));
  dialog_elements.dialog = dialog = gimp_dialog_new (title, "iterefocus",
                NULL, (GtkDialogFlags)(0),
                refocusit_help, PLUG_IN_PROC,
                _("_Preview"),  RESPONSE_PREVIEW,
                _("_Reset"),    RESPONSE_RESET,
                _("_Cancel"),   GTK_RESPONSE_CANCEL,
                _("_OK"),       GTK_RESPONSE_OK,
                NULL);
  g_free (title);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_PREVIEW,
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_CANCEL,
                                            GTK_RESPONSE_OK,
                                            -1);

  g_signal_connect (dialog, "response", G_CALLBACK (dialog_response), NULL);
  g_signal_connect_swapped (dialog, "destroy", G_CALLBACK (destroy_callback), NULL);

  preview_parameters_init ();
  dialog_parameters_create ();
  dialog_parameters_init ();

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  element = create_controls ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, TRUE, TRUE, 5);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  element = motion_angle_create ();
  gtk_box_pack_start (GTK_BOX (vbox), element, TRUE, FALSE, 5);

  element = preview_create ();
  gtk_box_pack_start (GTK_BOX (vbox), element, TRUE, FALSE, 5);

  gtk_widget_show (vbox);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 5);

  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog))), hbox, TRUE, FALSE, 5);

  gtk_widget_show (GTK_WIDGET (hbox));
  gtk_widget_show (dialog);

  preview_update ();
  dialog_elements_update ();

  motion_angle_draw (TRUE);
  gtk_main ();

  dialog_elements_destroy ();

  return dialog_parameters.frun;
}

static void get_lambdas (gdouble *lambda, gdouble *lambda_min) {
  *lambda_min = 1.0 / exp (input_parameters.lambda_min / 4.0);
  *lambda = input_parameters.lambda / LAMBDA_MAX * 0.001 / *lambda_min;
}

static void event_loop () {
  while (gtk_events_pending()) gtk_main_iteration_do(TRUE);
}

static void progress_bar_init () {
  char *title;

  title = g_strdup_printf (_("Refocusing..."));
  if (dialog_elements.progress) {
    dialog_elements.progress = gtk_progress_bar_new();
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog_elements.progress), title);
    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (dialog_elements.progress), TRUE);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog_elements.progress), 0.0);
  } else {
    gimp_progress_init (title);
  }
  g_free (title);
}

static void progress_bar_update (gdouble fraction) {
  if (dialog_elements.progress) {
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog_elements.progress), fraction);
  } else {
    gimp_progress_update (fraction);
  }

  event_loop ();
}

static void progress_bar_reset () {
  if (dialog_elements.progress) {
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog_elements.progress), 0.0);
  } else {
    gimp_progress_update (0.0);
  }
}

static int compute (int iterations) {
  int i;
  gdouble lambda_min, lambda;
  gdouble step, final;
  gboolean is_adaptive, is_smooth, is_mirror;
  convmask_t defoc, gauss, motion, blur;

  event_loop ();

  get_lambdas (&lambda, &lambda_min);

  is_smooth = (lambda > 1e-8 && lambda_min < LAMBDAMIN_USABLE_MAX);
  is_adaptive = (input_parameters.adaptive_smooth && is_smooth);
  is_mirror = (input_parameters.boundary == BOUNDARY_MIRROR);

  /* PROGRESS BAR */
  step = 1.0;
  final = (gdouble)(iterations);
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

  if (blur_create_defocus (&defoc, (gdouble)(input_parameters.radius)) == NULL) goto compute_err0;
  if (blur_create_gauss (&gauss, (gdouble)(input_parameters.gauss)) == NULL) goto compute_err1;
  if (blur_create_motion (&motion, (gdouble)(input_parameters.motion), (gdouble)(input_parameters.mot_angle)) == NULL) goto compute_err2;
  if (convmask_convolve (&blur, &defoc, &gauss) == NULL)  goto compute_err3;
  if (convmask_convolve (&hopfield.blur, &blur, &motion) == NULL) goto compute_err4;
  convmask_destroy (&blur);
  convmask_destroy (&motion);
  convmask_destroy (&gauss);
  convmask_destroy (&defoc);
#if defined(NDEBUG)
  int x, y, r;
  printf ("combine blur+motion+guass+defocus using convmask_convolve()");
  convmask_print (&hopfield.blur, "hopfield.blur");
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
    printf ("new, is_smooth, lambda_create(), x=%d y=%d lambda=%g lambda_min=%g x=%d y=%d winsize=%d combined radius=%d\n", x, y, lambda, lambda_min, hopfield.lambdafldR.x, hopfield.lambdafldR.y, input_parameters.winsize, r);
    printf ("hopfield.lambdafldR, hopfield.imageR\n");
    for (y = 0; y <= 8; y++) {
      for (x = 0; x <= 8; x++) {
        printf ("|%d %d %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x]);
      }
      printf ("\n");
    }
    convmask_print (&hopfield.filter, "hopfield.filter");
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
    printf ("..did smooth (before !is_adaptive)\n");
#endif

    if (!is_adaptive) {
      if (lambda_calculate (&hopfield.lambdafldR, &hopfield.imageR) == NULL) goto compute_err9;
      progress_bar_update (step++ / final);
#if defined(NDEBUG)
    x = hopfield.lambdafldR.x;
    y = hopfield.lambdafldR.y;
    printf ("!is_adaptive, lambda_calculate(), x=%d y=%d, hopfield.lambdafldR.lambda[] hopfield.imageR[]\n", x, y);
    for (y = 0; y <= 8; y++) {
      for (x = 0; x <= 8; x++) {
        printf ("|%d %d %f %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x], image_get(&hopfield.imageR,x,y) );
      }
      printf ("\n");
    }
#endif
      if (image_parameters.rgb) {
        if (lambda_calculate (&hopfield.lambdafldG, &hopfield.imageG) == NULL) goto compute_err9;
        progress_bar_update (step++ / final);
        if (lambda_calculate (&hopfield.lambdafldB, &hopfield.imageB) == NULL) goto compute_err9;
        progress_bar_update (step++ / final);
      }
#if defined(NDEBUG)
      printf ("..did !is_adaptive, lambda=%g\n", lambda);
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
  printf ("is_smooth, hopfield_create(), x=%d y=%d\n", x, y);
  printf ("hopfield.lambdafldR.lamba[], hopfield.imageR[]\n");
  for (y = 0; y <= 8; y++) {
    for (x = 0; x <= 8; x++) {
      printf ("|%d %d %f %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x], image_get (&hopfield.imageR,x,y) );
    }
    printf ("\n");
  }
  printf ("weights=");
  weights_print (&hopfield.hopfieldR.weights, "hopfield.blur");
  convmask_print (&hopfield.blur, "hopfield.blur");
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
  /* if image uses 0..255 or 0.0..1.0, weights,blur,lamba */
  /* come out to be equal value, others differ by ~16025. */
  printf ("{weights,blur,lambda}=same,imageR=0..255vs0..1\n");
  printf ("..did lambda = %g, now do iterations=%d\n", lambda, iterations);
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

#if defined(NDEBUG)
  x = hopfield.lambdafldR.x;
  y = hopfield.lambdafldR.y;
  printf ("iteration=%d, hopfield_iteration(), x=%d y=%d\n", i, x, y);
  printf ("hopfield.lambdafldR.lamba[], hopfield.imageR[]\n");
  for (y = 0; y <= 8; y++) {
    for (x = 0; x <= 8; x++) {
      printf ("|%d %d %f %f",x,y, hopfield.lambdafldR.lambda[hopfield.lambdafldR.x * y + x], image_get (&hopfield.imageR,x,y) );
    }
    printf ("\n");
  }
  printf ("weights=");
  weights_print (&hopfield.hopfieldR.weights, "hopfield.blur");
  convmask_print (&hopfield.blur, "hopfield.blur");
#endif

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

static GimpValueArray *
refocusit_run (GimpProcedure       *procedure,
               GimpRunMode          run_mode,  /* Current run mode */
               GimpImage           *image,
               GimpDrawable       **drawables,
               GimpProcedureConfig *proc_config,
               gpointer             run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);
#endif

  if (gimp_core_object_array_get_length ((GObject **)(drawables)) != 1) {
    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                 _("Procedure '%s' only works with one drawable."),
                 PLUG_IN_PROC);

    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             error);
  }
  /* Initialize parameter data... */
  if (image_parameters_init (drawables[0])) {
    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                 _("Procedure '%s' only works with RGB or GRAY images."),
                 PLUG_IN_PROC);

    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             error);
  }
  input_parameters_reset ();

  /* Load image data... */
  if (hopfield_data_init ()) {
    /* ...must be a very, very large image to stop at this point!!! */
    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                 "Procedure '%s' ran out of memory! Please report as issue.",
                 PLUG_IN_PROC);

    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             error);
  }
  hopfield_data_load ();

  /* See how we will run */
  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    /*INIT_I18N_UI();*/
    input_parameters_fetch_params (proc_config);
    //input_parameters_load ();
    gimp_ui_init (PLUG_IN_BINARY);
    if (dialog ()) {
//      input_parameters_save ();
    }
    gdk_display_flush (gdk_display_get_default ());
    break;

  case GIMP_RUN_NONINTERACTIVE:
    /*INIT_I18N();*/
//    if (nparams != 11) status = GIMP_PDB_CALLING_ERROR;
//    else {
      input_parameters_fetch_params (proc_config);
      compute (input_parameters.iterations);
//    }
    break;

  case GIMP_RUN_WITH_LAST_VALS:
    /*INIT_I18N();*/
    input_parameters_fetch_params (proc_config);
    //input_parameters_load ();
    gimp_ui_init (PLUG_IN_BINARY);
    compute (input_parameters.iterations);
    gdk_display_flush (gdk_display_get_default ());
    break;

  default:
    status = GIMP_PDB_CALLING_ERROR;
    break;
  };

  /* Detach from the drawable... */
  hopfield_data_destroy ();
  image_parameters_destroy ();

  gegl_exit();
#if defined(NDEBUG)
  printf ("Program end.\n");
#endif
  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static void refocusit_help (const gchar *help_id, gpointer help_data) {
  gchar *longdesc = g_strdup_printf (_(PLUG_IN_LONG_DESC));
  gimp_message(_(longdesc));
  g_free (longdesc);
}
