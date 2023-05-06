#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "phatprivate.h"
#include "phatpad.h"

/* signals */
enum
{
    VALUE_CHANGED_SIGNAL,
    LAST_SIGNAL,
};

/* action states */
enum
{
    STATE_NORMAL,
    STATE_PRESSED,
    STATE_SLIDE,
    STATE_ENTRY,
    STATE_SCROLL,
};

static GtkHBoxClass* parent_class;
static int signals[LAST_SIGNAL];


static void phat_pad_class_init               (PhatPadClass* klass);
static void phat_pad_init                     (PhatPad* pad);
static void phat_pad_destroy                  (GtkObject* object);
static void phat_pad_realize                  (GtkWidget* widget);
static void phat_pad_unrealize                (GtkWidget* widget);
static void phat_pad_map                      (GtkWidget* widget);
static void phat_pad_unmap                    (GtkWidget* widget);
static void phat_pad_size_allocate            (GtkWidget* widget,
                                               GtkAllocation* allocation);
static gboolean phat_pad_expose               (GtkWidget* widget,
                                               GdkEventExpose* event);
static gboolean phat_pad_button_press         (GtkWidget* widget,
                                               GdkEventButton* event);
static gboolean phat_pad_motion_notify        (GtkWidget* widget,
                                               GdkEventMotion* event);
static gboolean phat_pad_configure_event                 (GtkWidget *widget, 
                                                          GdkEventConfigure *event);

GType phat_pad_get_type ( )
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhatPadClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_pad_class_init,
                NULL,
                NULL,
                sizeof (PhatPad),
                0,
                (GInstanceInitFunc) phat_pad_init,
            };

        type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                       "PhatPad",
                                       &info,
                                       0);
    }

    return type;
}



/**
 * phat_pad_new:
 * 
 * Creates a new #PhatPad.
 *
 * Returns: a newly created #PhatPad
 * 
 */
GtkWidget* phat_pad_new ()
{
    PhatPad* pad;

    debug ("new\n");

    pad = g_object_new (PHAT_TYPE_PAD, NULL);
         
    return (GtkWidget*) pad;
}

static void phat_pad_class_init (PhatPadClass* klass)
{
    GtkObjectClass* object_class = (GtkObjectClass*) klass;
    GtkWidgetClass* widget_class = (GtkWidgetClass*) klass;

    debug ("class init\n");
     
    parent_class = gtk_type_class (gtk_drawing_area_get_type ());

    object_class->destroy = phat_pad_destroy;

    widget_class->realize = phat_pad_realize;
    widget_class->unrealize = phat_pad_unrealize;
    widget_class->map = phat_pad_map;
    widget_class->unmap = phat_pad_unmap;
    widget_class->size_allocate = phat_pad_size_allocate;
    widget_class->expose_event = phat_pad_expose;
    widget_class->button_press_event = phat_pad_button_press;
    widget_class->motion_notify_event = phat_pad_motion_notify;

    /**
     * PhatPad::value-changed:
     * @pad: the object on which the signal was emitted
     *
     * The "value-changed" signal is emitted when the value of the
     * pad's adjustment changes.
     *
     */
    
    signals[VALUE_CHANGED_SIGNAL] =
        g_signal_new ("value-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhatPadClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
     
    klass->value_changed = NULL;
}



static void phat_pad_init (PhatPad* pad)
{
    debug ("init\n");

    GtkWidget* widget = GTK_WIDGET (pad);
    int focus_width, focus_pad;
    
    
    GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
     
    /* our parent class sets this to false; we need it to be true so
     * that we are drawn without glitches when first shown
     * (mapped) */
    //gtk_widget_set_redraw_on_allocate (GTK_WIDGET (box), TRUE);
    
    pad->x = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 20000.0, 0.1, 0.1, 0.1);
    pad->y = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 20000.0, 0.1, 0.1, 0.1);
    pad->xtilt = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1.0, 0.1, 0.1, 0.1);
    pad->ytilt = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1.0, 0.1, 0.1, 0.1);
    pad->pressure = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1.0, 0.1, 0.1, 0.1);
    pad->x_is_log = 0;
    pad->y_is_log = 0;
    pad->p_is_log = 0;
    pad->xt_is_log = 0;
    pad->yt_is_log = 0;
  
    
        
    pad->pixmap = NULL;
    gtk_widget_set_size_request (GTK_WIDGET (pad), 200, 200);
         

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);
    
    g_signal_connect (G_OBJECT (pad), "expose_event",
                      G_CALLBACK (phat_pad_expose), NULL);
    g_signal_connect (G_OBJECT(pad),"configure_event",
                      G_CALLBACK (phat_pad_configure_event), NULL);

    /* Event signals */

    g_signal_connect (G_OBJECT (pad), "motion_notify_event",
                      G_CALLBACK (phat_pad_motion_notify), NULL);
    g_signal_connect (G_OBJECT (pad), "button_press_event",
                      G_CALLBACK (phat_pad_button_press), NULL);

    gtk_widget_set_events (widget, GDK_EXPOSURE_MASK
                           | GDK_LEAVE_NOTIFY_MASK
                           | GDK_BUTTON_PRESS_MASK
                           | GDK_POINTER_MOTION_MASK
                           | GDK_POINTER_MOTION_HINT_MASK);

    /* The following call enables tracking and processing of extension
       events for the drawing area */
    gtk_widget_set_extension_events (widget, GDK_EXTENSION_EVENTS_CURSOR);
    
}



static void phat_pad_destroy (GtkObject* object)
{
    GtkObjectClass* klass;
    PhatPad* pad;
    GtkWidget* widget;
     
    debug ("destroy %p\n", object);
     
    g_return_if_fail (object != NULL);
    g_return_if_fail (PHAT_IS_PAD(object));

    klass = GTK_OBJECT_CLASS (parent_class);
    pad = (PhatPad*) object;
    widget = GTK_WIDGET (object);

   
    if (pad->pixmap)
    {
        //gtk_widget_destroy (pad->pixmap);
        pad->pixmap = NULL;
    }
      
    if (klass->destroy)
        klass->destroy (object);
}



static void phat_pad_realize (GtkWidget* widget)
{
    GtkWidgetClass* klass = GTK_WIDGET_CLASS (parent_class);
    //PhatPad* pad = (PhatPad*) widget;
        
    debug ("realize\n");
     
    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHAT_IS_PAD(widget));

    if (klass->realize)
        klass->realize (widget);

}



static void phat_pad_unrealize (GtkWidget *widget)
{
    //PhatPad* pad = PHAT_PAD(widget);
    GtkWidgetClass* klass = GTK_WIDGET_CLASS (parent_class);

    debug ("unrealize\n");
     
     
    if (klass->unrealize)
        klass->unrealize (widget);
}


static void phat_pad_map (GtkWidget *widget)
{
    PhatPad* pad;

    debug ("map\n");
     
    g_return_if_fail (PHAT_IS_PAD(widget));
    pad = (PhatPad*) widget;
    //debug ("during map\n");

    GTK_WIDGET_CLASS (parent_class)->map (widget);

    gtk_widget_queue_draw (widget);
    //debug ("after map\n");

}


static void phat_pad_unmap (GtkWidget *widget)
{
    PhatPad* pad;

    debug ("unmap\n");
     
    g_return_if_fail (PHAT_IS_PAD(widget));
    pad = (PhatPad*) widget;
   
    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


static void phat_pad_size_allocate (GtkWidget* widget,
                                    GtkAllocation* allocation)
{
    PhatPad* pad;
    pad = PHAT_PAD(widget);

    g_return_if_fail (widget != NULL);
    g_return_if_fail (allocation != NULL);
    g_return_if_fail (PHAT_IS_PAD(widget));

    debug ("size allocate\n");
     
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

    if (GTK_WIDGET_REALIZED (widget))
    {
        debug ("setting pixmap size\n");
        //if (pad->pixmap)
        //  g_object_unref (pad->pixmap);

        /*
          pad->pixmap = gdk_pixmap_new (pad->drawing_area->window,
          widget->allocation.width,
          widget->allocation.height,
          -1);
          gdk_draw_rectangle (pad->drawing_area->window,
          widget->style->black_gc,
          TRUE,
          0, 0,
          widget->allocation.width,
          widget->allocation.height);
        */
        /*gdk_window_move_resize (PHAT_PAD(widget)->event_window,
          allocation->x,
          allocation->y,
          allocation->width,
          allocation->height);*/
    }
}

static gboolean phat_pad_expose (GtkWidget*      widget,
                                 GdkEventExpose* event)
{
    PhatPad* pad;
    GtkAllocation* a;
     
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_PAD(widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (GTK_WIDGET_DRAWABLE (widget), FALSE);
    g_return_val_if_fail (event->count == 0, FALSE);

    debug ("expose\n");

    pad = PHAT_PAD(widget);
    a = &widget->allocation;

    gdk_draw_drawable (widget->window,
                       widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                       pad->pixmap,
                       event->area.x, event->area.y,
                       event->area.x, event->area.y,
                       event->area.width, event->area.height);
   
    //GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
    
 
    return TRUE;
}

gboolean draw_freq (GtkWidget *widget)
{
    PhatPad* pad;
    gdouble initfreq = 16.351;
    gdouble freq = initfreq;
    gdouble x;

    debug ("draw freq  \n");
    pad = PHAT_PAD(widget);  
    
    if(pad->x_is_log)
    {
        debug ("x is log  \n");

        while(freq < pad->x->upper)
        {
            freq = freq * 1.0594631;
            if(freq > pad->x->lower)
            {
                x = log(freq - pad->x->lower) / log(pad->x->upper - pad->x->lower) * widget->allocation.width;
                gdk_draw_line (pad->pixmap, widget->style->white_gc, x, 0, x, widget->allocation.height);
            }
        }
    }
    else
    {
        while(freq < pad->x->upper)
        {
            freq = freq * 1.0594631;
            if(freq > pad->x->lower)
            {
                x = log(freq - pad->x->lower) / log(pad->x->upper - pad->x->lower) * widget->allocation.width;
                gdk_draw_line (pad->pixmap, widget->style->white_gc, x, 0, x, widget->allocation.height);
            }
        }
    }    
    return TRUE;
}


/* Create a new backing pixmap of the appropriate size */
static gboolean phat_pad_configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
    PhatPad* pad;

    debug ("configure  \n");
    pad = PHAT_PAD(widget);
    if (pad->pixmap)
        g_object_unref (pad->pixmap);

    pad->pixmap = gdk_pixmap_new (widget->window,
                                  widget->allocation.width,
                                  widget->allocation.height,
                                  -1);
    gdk_draw_rectangle (pad->pixmap,
                        widget->style->black_gc,
                        TRUE,
                        0, 0,
                        widget->allocation.width,
                        widget->allocation.height);
    draw_freq(widget);

    return TRUE;
}

static gboolean phat_pad_button_press (GtkWidget* widget,
                                       GdkEventButton* event)
{
    
    PhatPad* pad = PHAT_PAD(widget);
    gdouble temppressure, tempxtilt, tempytilt;
         
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    if (event->button == 1 && pad->pixmap != NULL) {
        debug ("pad press\n");
        if(pad->x_is_log)
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->x, exp((event->x / widget->allocation.width) * 
                                                                  (log(pad->x->upper - pad->x->lower))) + pad->x->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->x, (event->x / widget->allocation.width) * (pad->x->upper - pad->x->lower) + pad->x->lower);
        }
        if(pad->y_is_log)   
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->y, exp((event->y / widget->allocation.height) * 
                                                                  (log(pad->y->upper - pad->y->lower))) + pad->y->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->y, (event->y / widget->allocation.height) * (pad->y->upper - pad->y->lower) + pad->y->lower);
        }
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_PRESSURE, &temppressure);
        if(pad->p_is_log)
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->pressure, exp((temppressure) * (log(pad->pressure->upper - pad->pressure->lower))) + pad->pressure->lower);

        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->pressure, (temppressure * (pad->pressure->upper - pad->pressure->lower) + pad->pressure->lower));
        }

        
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_XTILT, &tempxtilt);
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_YTILT, &tempytilt);
        return TRUE;

        //draw_brush (widget, event->device->source, event->x, event->y, pressure);
    }
    else
    {
        return FALSE;
    }
}


static gboolean phat_pad_motion_notify (GtkWidget* widget,
                                        GdkEventMotion* event)
{
    PhatPad* pad = PHAT_PAD(widget);
    gdouble tempx, tempy, temppressure, tempxtilt, tempytilt;

    GdkModifierType state;

    debug ("motion\n");

    //printf("xtilt min %f max %f \n", event->device->axes[GDK_AXIS_XTILT].min, event->device->axes[GDK_AXIS_XTILT].max);

    if (event->is_hint) 
    {
        gdk_device_get_state (event->device, event->window, NULL, &state);
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_X, &tempx);
        if(pad->x_is_log)
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->x, exp((tempx / widget->allocation.width) * 
                                                                  (log(pad->x->upper - pad->x->lower))) + pad->x->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->x, (tempx / widget->allocation.width) * (pad->x->upper - pad->x->lower) + pad->x->lower);
        }       
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_Y, &tempy);
        if(pad->y_is_log)
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->y, exp((tempy / widget->allocation.height) * 
                                                                  (log(pad->y->upper - pad->y->lower))) + pad->y->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->y, (tempy / widget->allocation.height) * (pad->y->upper - pad->y->lower) + pad->y->lower);
        }
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_PRESSURE, &temppressure);
        if(pad->p_is_log)
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->pressure, exp((temppressure) * 
                                                                         (log(pad->pressure->upper - pad->pressure->lower))) + pad->pressure->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->pressure, (temppressure * (pad->pressure->upper - pad->pressure->lower) + pad->pressure->lower));
        }

        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_XTILT, &tempxtilt);
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_YTILT, &tempytilt);
    }
    else
    {
        if(pad->x_is_log)   
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->x, exp((event->x / widget->allocation.width) * 
                                                                  (log(pad->x->upper - pad->x->lower))) + pad->x->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->x, (event->x / widget->allocation.width) * (pad->x->upper - pad->x->lower) + pad->x->lower);
        }
        if(pad->y_is_log)   
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->y, exp((event->y / widget->allocation.height) * 
                                                                  (log(pad->y->upper - pad->y->lower))) + pad->y->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->y, (event->y / widget->allocation.height) * (pad->y->upper - pad->y->lower) + pad->y->lower);
        }
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_PRESSURE, &temppressure);
        if(pad->p_is_log)
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->pressure, exp((temppressure) * 
                                                                         (log(pad->pressure->upper - pad->pressure->lower))) + pad->pressure->lower);
        } 
        else 
        {
            gtk_adjustment_set_value((GtkAdjustment *)pad->pressure, (temppressure * (pad->pressure->upper - pad->pressure->lower) + pad->pressure->lower));
        }

        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_XTILT, &tempxtilt);
        gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_YTILT, &tempytilt);
        state = event->state;
    }
        
    //printf("pressure %f x %f y %f\n", pad->pressure, pad->x, pad->y);
    
    g_signal_emit (G_OBJECT (widget), signals[VALUE_CHANGED_SIGNAL], 0);
    //if (state & GDK_BUTTON1_MASK && widget->pixmap != NULL)
    //phat_pad_get_xphat_pad_get_xdraw_brush (widget, event->device->source, x, y, pressure);
      
    return TRUE;
}

/**
 * phat_pad_get_x:
 * @pad: a #PhatPad
 *
 * Retrieves the current x value of the pad.
 *
 * Returns: current x value.
 *
 */
GtkAdjustment* phat_pad_get_x (PhatPad* pad)
{
    g_return_val_if_fail (PHAT_IS_PAD (pad), 0);

    return (GtkAdjustment *)pad->x;

}

/**
 * phat_pad_set_x_range:
 * @pad: a #PhatPad
 *
 * Sets the x range of the pad.
 *
 *
 */
void phat_pad_set_x_range (PhatPad* pad, gdouble min, gdouble max)
{
    pad->x->lower = min;
    pad->x->upper = max;
    return;
}

void phat_pad_set_x_log (PhatPad* pad, gboolean is_log)
{
    pad->x_is_log = is_log;
    return;
}

void phat_pad_set_y_log (PhatPad* pad, gboolean is_log)
{
    pad->y_is_log = is_log;
    return;
}

void phat_pad_set_xtilt_log (PhatPad* pad, gboolean is_log)
{
    pad->xt_is_log = is_log;
    return;
}

void phat_pad_set_ytilt_log (PhatPad* pad, gboolean is_log)
{
    pad->yt_is_log = is_log;
    return;
}

void phat_pad_set_pressure_log (PhatPad* pad, gboolean is_log)
{
    pad->p_is_log = is_log;
    return;
}

gboolean phat_pad_x_is_log (PhatPad* pad)
{
    return pad->x_is_log;
}

gboolean phat_pad_y_is_log (PhatPad* pad)
{
    return pad->y_is_log;
}

gboolean phat_pad_xtilt_is_log (PhatPad* pad)
{
    return pad->xt_is_log;
}

gboolean phat_pad_ytilt_is_log (PhatPad* pad)
{
    return pad->yt_is_log;
}

gboolean phat_pad_pressure_is_log (PhatPad* pad)
{
    return pad->p_is_log;
}


void phat_pad_set_y_range (PhatPad* pad, gdouble min, gdouble max)
{
    pad->y->lower = min;
    pad->y->upper = max;
    return;
}

void phat_pad_set_pressure_range (PhatPad* pad, gdouble min, gdouble max)
{
    pad->pressure->lower = min;
    pad->pressure->upper = max;
    return;
}


/**
 * phat_pad_get_y:
 * @pad: a #PhatPad
 *
 * Retrieves the current y value of the pad.
 *
 * Returns: current y value.
 *
 */
GtkAdjustment* phat_pad_get_y (PhatPad* pad)
{
    g_return_val_if_fail (PHAT_IS_PAD (pad), 0);

    return (GtkAdjustment *)pad->y;
}

/**
 * phat_pad_get_pressure:
 * @pad: a #PhatPad
 *
 * Retrieves the current pressure value of the pad.
 *
 * Returns: current pressure value.
 *
 */
GtkAdjustment* phat_pad_get_pressure (PhatPad* pad)
{
    g_return_val_if_fail (PHAT_IS_PAD (pad), 0);

    return (GtkAdjustment *)pad->pressure;
}

/**
 * phat_pad_get_xtilt:
 * @pad: a #PhatPad
 *
 * Retrieves the current xtilt value of the pad.
 *
 * Returns: current xtilt value.
 *
 */
GtkAdjustment* phat_pad_get_xtilt (PhatPad* pad)
{
    g_return_val_if_fail (PHAT_IS_PAD (pad), 0);
    return (GtkAdjustment *)pad->xtilt;

}


/**
 * phat_pad_get_ytilt:
 * @pad: a #PhatPad
 *
 * Retrieves the current ytilt value of the pad.
 *
 * Returns: current ytilt value.
 *
 */
GtkAdjustment* phat_pad_get_ytilt (PhatPad* pad)
{
    g_return_val_if_fail (PHAT_IS_PAD (pad), 0);

    return (GtkAdjustment *)pad->ytilt;

}
