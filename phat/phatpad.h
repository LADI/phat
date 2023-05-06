#ifndef __PHAT_PAD_H__
#define __PHAT_PAD_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PHAT_TYPE_PAD            (phat_pad_get_type ( ))
#define PHAT_PAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PHAT_TYPE_PAD, PhatPad))
#define PHAT_PAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PHAT_TYPE_PAD, PhatPadClass))
#define PHAT_IS_PAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PHAT_TYPE_PAD))
#define PHAT_IS_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PHAT_TYPE_PAD))

typedef struct _PhatPadClass PhatPadClass;
typedef struct _PhatPad      PhatPad;

struct _PhatPad
{
    GtkDrawingArea parent; 
    GdkPixmap* pixmap;
    GtkAdjustment* pressure;
    gboolean p_is_log;
    GtkAdjustment* xtilt;
    gboolean xt_is_log;
    GtkAdjustment* ytilt;
    gboolean yt_is_log;
    GtkAdjustment* x;
    gboolean x_is_log;
    GtkAdjustment* y;
    gboolean y_is_log;
};

struct _PhatPadClass
{
    GtkDrawingAreaClass parent_class;

    void (*value_changed) (PhatPad* pad);
};

GType phat_pad_get_type ( );


GtkWidget* phat_pad_new ( );
GtkAdjustment* phat_pad_get_ytilt (PhatPad* pad);
GtkAdjustment* phat_pad_get_xtilt (PhatPad* pad);
GtkAdjustment* phat_pad_get_pressure (PhatPad* pad);
GtkAdjustment* phat_pad_get_y (PhatPad* pad);
GtkAdjustment* phat_pad_get_x (PhatPad* pad);
void phat_pad_set_x_range (PhatPad* pad, gdouble min, gdouble max);
void phat_pad_set_y_range (PhatPad* pad, gdouble min, gdouble max);
void phat_pad_set_pressure_range (PhatPad* pad, gdouble min, gdouble max);
void phat_pad_set_x_log (PhatPad* pad, gboolean is_log);
void phat_pad_set_y_log (PhatPad* pad, gboolean is_log);
void phat_pad_set_xtilt_log (PhatPad* pad, gboolean is_log);
void phat_pad_set_ytilt_log (PhatPad* pad, gboolean is_log);
void phat_pad_set_pressure_log (PhatPad* pad, gboolean is_log);
gboolean phat_pad_x_is_log (PhatPad* pad);
gboolean phat_pad_y_is_log (PhatPad* pad);
gboolean phat_pad_xtilt_is_log (PhatPad* pad);
gboolean phat_pad_ytilt_is_log (PhatPad* pad);
gboolean phat_pad_pressure_is_log (PhatPad* pad);

G_END_DECLS

#endif /* __PHAT_PAD_H__ */
