#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "phatprivate.h"
#include "phatfanslider.h"

/* magic numbers */
enum
{
    FAN_RISE = 3,
    FAN_RUN = 1,
    SLIDER_WIDTH = 16,
    SLIDER_LENGTH = 32,
    THRESHOLD = 4,
};

/* states */
enum
{
    STATE_NORMAL,
    STATE_CLICKED,
    STATE_SCROLL,
};

/* signals */
enum
{
    VALUE_CHANGED_SIGNAL,
    CHANGED_SIGNAL,
    LAST_SIGNAL,
};

static int phat_fan_slider_signals [LAST_SIGNAL] = { 0, 0 };

static GtkWidgetClass* parent_class   = NULL;
static int             fan_max_height = 0;
static int             fan_max_width  = 0;

static void phat_fan_slider_class_init      (PhatFanSliderClass* klass);
static void phat_fan_slider_init            (PhatFanSlider* slider);
static void phat_fan_slider_destroy         (GtkObject* object);
static void phat_fan_slider_realize         (GtkWidget* widget);
static void phat_fan_slider_unrealize       (GtkWidget *widget);
static void phat_fan_slider_map             (GtkWidget *widget);
static void phat_fan_slider_unmap           (GtkWidget *widget);
static void phat_fan_slider_draw_fan        (PhatFanSlider* slider);
static int  phat_fan_slider_get_fan_length  (PhatFanSlider* slider);
static void phat_fan_slider_build_fan_clips (PhatFanSlider* slider);
static void phat_fan_slider_update_hints    (PhatFanSlider* slider);

static void phat_fan_slider_size_request (GtkWidget*widget,
                                          GtkRequisition* requisition);

static void phat_fan_slider_size_allocate (GtkWidget* widget,
                                           GtkAllocation* allocation);

static gboolean phat_fan_slider_expose (GtkWidget* widget,
                                        GdkEventExpose* event);

static gboolean phat_fan_slider_button_press (GtkWidget* widget,
                                              GdkEventButton* event);

static gboolean phat_fan_slider_button_release (GtkWidget* widget,
                                                GdkEventButton* event);

static gboolean phat_fan_slider_key_press (GtkWidget* widget,
                                           GdkEventKey* event);

static gboolean phat_fan_slider_scroll (GtkWidget* widget,
                                        GdkEventScroll* event);

static gboolean phat_fan_slider_motion_notify (GtkWidget* widget,
                                               GdkEventMotion* event);

static gboolean phat_fan_slider_enter_notify (GtkWidget* widget,
                                              GdkEventCrossing* event);

static gboolean phat_fan_slider_leave_notify (GtkWidget* widget,
                                              GdkEventCrossing* event);

static void phat_fan_slider_calc_layout (PhatFanSlider* slider,
                                         int* x, int* y, int* w, int* h);

static void phat_fan_slider_update_value (PhatFanSlider* slider,
                                          int x_root, int y_root);

static void phat_fan_slider_update_fan (PhatFanSlider* slider,
                                        int x, int y);

static gboolean phat_fan_slider_fan_expose (GtkWidget*      widget,
                                            GdkEventExpose* event,
                                            PhatFanSlider*  slider);

static void phat_fan_slider_fan_show (GtkWidget* widget,
                                      GtkWidget* slider);

static gboolean phat_fan_slider_hint_expose (GtkWidget* widget,
                                             GdkEventExpose* event,
                                             GtkWidget* slider);

static void phat_fan_slider_adjustment_changed (GtkAdjustment* adjustment,
                                                PhatFanSlider* slider);

static void phat_fan_slider_adjustment_value_changed (GtkAdjustment* adjustment,
                                                      PhatFanSlider* slider);

GType phat_fan_slider_get_type ( )
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhatFanSliderClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_fan_slider_class_init,
                NULL,
                NULL,
                sizeof (PhatFanSlider),
                0,
                (GInstanceInitFunc) phat_fan_slider_init,
            };

        type = g_type_register_static (GTK_TYPE_WIDGET,
                                       "PhatFanSlider",
                                       &info,
                                       G_TYPE_FLAG_ABSTRACT);
    }

    return type;
}

/**
 * phat_fan_slider_set_value:
 * @slider: a #PhatFanSlider
 * @value: a new value for the slider
 * 
 * Sets the current value of the slider.  If the value is outside the
 * range of values allowed by @slider, it will be clamped to fit
 * within them.  The slider emits the "value-changed" signal if the
 * value changes.
 *
 */
void phat_fan_slider_set_value (PhatFanSlider* slider, double value)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    value = CLAMP (value,
                   slider->adjustment->lower,
                   slider->adjustment->upper);

    gtk_adjustment_set_value (slider->adjustment, value);

    if(slider->is_log)
    {           
        gtk_adjustment_set_value((GtkAdjustment *)slider->adjustment_prv, log(value - slider->adjustment->lower) / log(slider->adjustment->upper - slider->adjustment->lower));         
    }
    else
    {
        gtk_adjustment_set_value((GtkAdjustment *)slider->adjustment_prv, (value - slider->adjustment->lower) / (slider->adjustment->upper - slider->adjustment->lower));
    }
}

void phat_fan_slider_set_log (PhatFanSlider* slider, gboolean is_log)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    slider->is_log = is_log;
}

gboolean phat_fan_slider_is_log (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), 0);

    return slider->is_log;
}

    

/**
 * phat_fan_slider_get_value:
 * @slider: a #PhatFanSlider
 *
 * Retrieves the current value of the slider.
 *
 * Returns: current value of the slider.
 *
 */
double phat_fan_slider_get_value (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), 0);

    if(slider->is_log)
    {
        gtk_adjustment_set_value((GtkAdjustment *)slider->adjustment, exp((slider->adjustment_prv->value) * 
                                                                          (log(slider->adjustment->upper - slider->adjustment->lower))) + slider->adjustment->lower);
        //printf("setting val %f lower %f upper %f \n", slider->adjustment_prv->value, slider->adjustment->lower, slider->adjustment->upper);
    }
    else
    {
        gtk_adjustment_set_value((GtkAdjustment *)slider->adjustment, (slider->adjustment_prv->value * (slider->adjustment->upper - slider->adjustment->lower) + slider->adjustment->lower));
    }
    return slider->adjustment->value;
}

/**
 * phat_fan_slider_set_range:
 * @slider: a #PhatFanSlider
 * @lower: lowest allowable value
 * @upper: highest allowable value
 * 
 * Sets the range of allowable values for the slider, and  clamps the slider's
 * current value to be between @lower and @upper.
 *
 */
void phat_fan_slider_set_range (PhatFanSlider* slider,
                                double lower, double upper)
{
    double value;
     
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));
    g_return_if_fail (lower <= upper);

    slider->adjustment->lower = lower;
    slider->adjustment->upper = upper;

    value = CLAMP (slider->adjustment->value,
                   slider->adjustment->lower,
                   slider->adjustment->upper);
    //XXX not sure about these
    gtk_adjustment_changed (slider->adjustment);
    gtk_adjustment_set_value (slider->adjustment, value);
}

/**
 * phat_fan_slider_get_range:
 * @slider: a #PhatFanSlider
 * @lower: retrieves lowest allowable value
 * @upper: retrieves highest allowable value
 *
 * Places the range of allowable values for @slider into @lower
 * and @upper.  Either variable may be set to %NULL if you are not
 * interested in its value.
 *
 */
void phat_fan_slider_get_range (PhatFanSlider* slider,
                                double* lower, double* upper)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    if (lower)
        *lower = slider->adjustment->lower;
    if (upper)
        *upper = slider->adjustment->upper;
}

/**
 * phat_fan_slider_set_adjustment:
 * @slider: a #PhatFanSlider
 * @adjustment: a #GtkAdjustment
 *
 * Sets the adjustment used by @slider.  Every #PhatFanSlider uses an
 * adjustment to store its current value and its range of allowable
 * values.  If @adjustment is %NULL, a new adjustment with a value of
 * zero and a range of [-1.0, 1.0] will be created.
 *
 */
void phat_fan_slider_set_adjustment (PhatFanSlider* slider,
                                     GtkAdjustment* adjustment)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));
    g_return_if_fail (slider->adjustment != adjustment);

    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, -1.0, 1.0, 1.0, 1.0, 0.0);
    else
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

    if (slider->adjustment)
    {
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_changed,
                                              (gpointer) slider);
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_value_changed,
                                              (gpointer) slider);
        g_object_unref (slider->adjustment);
    }

    slider->adjustment = adjustment;
    g_object_ref (adjustment);
    gtk_object_sink (GTK_OBJECT (adjustment));

    phat_fan_slider_adjustment_changed(slider->adjustment, slider);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER (slider), adjustment->value);
}

/**
 * phat_fan_slider_get_adjustment:
 * @slider: a #PhatFanSlider
 *
 * Retrives the current adjustment in use by @slider.
 *
 * Returns: @slider's current #GtkAdjustment
 *
 */
GtkAdjustment* phat_fan_slider_get_adjustment (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), NULL);

    /* I can't imagine this ever being true, but just to be
     * "safe" ... */
    if (!slider->adjustment)
        phat_fan_slider_set_adjustment (slider, NULL);

    return slider->adjustment;
}

/**
 * phat_fan_slider_set_inverted:
 * @slider: a #PhatFanSlider
 * @inverted: %TRUE to invert the fanslider
 *  
 * Sets in which direction the fanslider should draw increasing
 * values.  By default, horizontal fansliders draw low to high from
 * left to right, and vertical fansliders draw from bottom to top.
 * You can reverse this behavior by setting @inverted to %TRUE.
 * 
 */
void phat_fan_slider_set_inverted (PhatFanSlider* slider, gboolean inverted)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    slider->inverted = inverted;
    gtk_widget_queue_draw (GTK_WIDGET (slider));
}

/**
 * phat_fan_slider_get_inverted:
 * @slider: a #PhatFanSlider
 *
 * Determines whether @slider is inverted or not.
 *
 * Returns: %TRUE if @slider is inverted
 *
 */
gboolean phat_fan_slider_get_inverted (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), FALSE);

    return slider->inverted;
}

/**
 * phat_fan_slider_set_default_value:
 * @slider: a #PhatFanSlider
 * @value: the default value
 *  
 * Set default value of the slider. Slider is reset to this value
 * when middle mouse button is pressed.
 */
void phat_fan_slider_set_default_value(PhatFanSlider* slider, gdouble value)
{
    g_return_if_fail(PHAT_IS_FAN_SLIDER(slider));

    slider->use_default_value = TRUE;
    slider->default_value = value;
}

static void phat_fan_slider_class_init (PhatFanSliderClass* klass)
{
    GtkObjectClass* object_class = (GtkObjectClass*) klass;
    GtkWidgetClass* widget_class = (GtkWidgetClass*) klass;
    GdkScreen*      screen       = gdk_screen_get_default ( );

    debug ("class init\n");
     
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = phat_fan_slider_destroy;

    widget_class->realize = phat_fan_slider_realize;
    widget_class->unrealize = phat_fan_slider_unrealize;
    widget_class->map = phat_fan_slider_map;
    widget_class->unmap = phat_fan_slider_unmap;
    widget_class->expose_event = phat_fan_slider_expose;
    widget_class->size_request = phat_fan_slider_size_request;
    widget_class->size_allocate = phat_fan_slider_size_allocate;
    widget_class->button_press_event = phat_fan_slider_button_press;
    widget_class->button_release_event = phat_fan_slider_button_release;
    widget_class->key_press_event = phat_fan_slider_key_press;
    widget_class->scroll_event = phat_fan_slider_scroll;
    widget_class->motion_notify_event = phat_fan_slider_motion_notify;
    widget_class->enter_notify_event = phat_fan_slider_enter_notify;
    widget_class->leave_notify_event = phat_fan_slider_leave_notify;
     
    /**
     * PhatFanSlider::value-changed:
     * @slider: the object on which the signal was emitted
     *
     * The "value-changed" signal is emitted when the value of the
     * slider's adjustment changes.
     *
     */
    phat_fan_slider_signals[VALUE_CHANGED_SIGNAL] =
        g_signal_new ("value-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhatFanSliderClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /**
     * PhatFanSlider::changed:
     * @slider: the object on which the signal was emitted
     *
     * The "changed" signal is emitted when any parameter of the
     * slider's adjustment changes, except for the %value parameter.
     *
     */
    phat_fan_slider_signals[CHANGED_SIGNAL] =
        g_signal_new ("changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhatFanSliderClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    klass->value_changed = NULL;
    klass->changed = NULL;

    if (screen)
        fan_max_width = gdk_screen_get_width (screen);
    else
        fan_max_width = 1280;

    if (screen)
        fan_max_height = gdk_screen_get_height (screen);
    else
        fan_max_height = 1024;
}

static void phat_fan_slider_init (PhatFanSlider* slider)
{
    GTK_WIDGET_SET_FLAGS (slider, GTK_NO_WINDOW | GTK_CAN_FOCUS);

    debug ("init\n");

    slider->adjustment = NULL;
    slider->adjustment_prv = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.1, 0.0);
    slider->val = 0.69;
    slider->center_val = -1;
    slider->xclick_root = 0;
    slider->yclick_root = 0;
    slider->xclick = 0;
    slider->yclick = 0;
    slider->fan_max_thickness = 1;
    slider->inverted = FALSE;
    slider->state = STATE_NORMAL;
    slider->orientation = GTK_ORIENTATION_HORIZONTAL;
    slider->fan_window = NULL;
    slider->fan_clip0 = NULL;
    slider->fan_clip1 = NULL;
    slider->arrow_cursor = NULL;
    slider->empty_cursor = NULL;
    slider->event_window = NULL;
    slider->hint_window0 = NULL;
    slider->hint_window1 = NULL;
    slider->hint_clip0 = NULL;
    slider->hint_clip1 = NULL;
    slider->is_log = 0;
    slider->use_default_value = FALSE;

    g_signal_connect (slider->adjustment_prv, "changed",
                      G_CALLBACK (phat_fan_slider_adjustment_changed),
                      (gpointer) slider);
    g_signal_connect (slider->adjustment_prv, "value_changed",
                      G_CALLBACK (phat_fan_slider_adjustment_value_changed),
                      (gpointer) slider);

    phat_fan_slider_adjustment_changed (slider->adjustment_prv, slider);
    phat_fan_slider_adjustment_value_changed (slider->adjustment_prv, slider);

}

static void phat_fan_slider_destroy (GtkObject* object)
{
    GtkObjectClass* klass;
    PhatFanSlider* slider;
    GtkWidget* widget;
     
    debug ("destroy %p\n", object);
     
    g_return_if_fail (object != NULL);
    g_return_if_fail (PHAT_IS_FAN_SLIDER (object));

    klass = GTK_OBJECT_CLASS (parent_class);
    slider = (PhatFanSlider*) object;
    widget = GTK_WIDGET (object);

    if (slider->arrow_cursor != NULL)
    {
        gdk_cursor_unref (slider->arrow_cursor);
        slider->arrow_cursor = NULL;
    }

    if (slider->empty_cursor != NULL)
    {
        gdk_cursor_unref (slider->empty_cursor);
        slider->empty_cursor = NULL;
    }

    if (slider->event_window != NULL)
    {
        gdk_window_destroy (slider->event_window);
        slider->event_window = NULL;
    }

    if (slider->fan_window != NULL)
    {
        gtk_widget_destroy (slider->fan_window);
        slider->fan_window = NULL;
    }

    if (slider->fan_clip0 != NULL)
    {
        gdk_region_destroy (slider->fan_clip0);
        slider->fan_clip0 = NULL;
    }

    if (slider->fan_clip1 != NULL)
    {
        gdk_region_destroy (slider->fan_clip1);
        slider->fan_clip1 = NULL;
    }

    if (slider->hint_window0 != NULL)
    {
        gtk_widget_destroy (slider->hint_window0);
        slider->hint_window0 = NULL;
    }

    if (slider->hint_window1 != NULL)
    {
        gtk_widget_destroy (slider->hint_window1);
        slider->hint_window1 = NULL;
    }

    if (slider->hint_clip0 != NULL)
    {
        gdk_region_destroy (slider->hint_clip0);
        slider->hint_clip0 = NULL;
    }

    if (slider->hint_clip1 != NULL)
    {
        gdk_region_destroy (slider->hint_clip1);
        slider->hint_clip1 = NULL;
    }

    if (slider->adjustment)
    {
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_changed,
                                              (gpointer) slider);
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_value_changed,
                                              (gpointer) slider);
        // called ref on it so we must call unref
        g_object_unref (slider->adjustment);
        slider->adjustment = NULL;
    }

    if (slider->adjustment_prv)
    {
        g_signal_handlers_disconnect_by_func (slider->adjustment_prv,
                                              phat_fan_slider_adjustment_changed,
                                              (gpointer) slider);
        g_signal_handlers_disconnect_by_func (slider->adjustment_prv,
                                              phat_fan_slider_adjustment_value_changed,
                                              (gpointer) slider);
        //didn't call ref on this one so just destroy
        gtk_object_destroy ((GtkObject*)slider->adjustment_prv);
        slider->adjustment_prv = NULL;
    }

     
    if (klass->destroy)
        klass->destroy (object);
}

static void phat_fan_slider_realize (GtkWidget* widget)
{
    PhatFanSlider* slider;
    GdkWindowAttr attributes;
    GdkPixmap* pixmap;
    GdkColor color = { 0, 0, 0, 0 };
    gchar data = 0;
    gint attributes_mask;

    debug ("realize\n");
     
    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    slider = (PhatFanSlider*) widget;
     
    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        slider->arrow_cursor = gdk_cursor_new (GDK_SB_V_DOUBLE_ARROW);
    }
    else
    {
        slider->arrow_cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    }

    pixmap = gdk_bitmap_create_from_data (NULL, &data, 1, 1);
    slider->empty_cursor = gdk_cursor_new_from_pixmap (pixmap, pixmap,
                                                       &color, &color,
                                                       0, 0);
    gdk_pixmap_unref (pixmap);

    widget->window = gtk_widget_get_parent_window (widget);
    g_object_ref (widget->window);
    widget->style = gtk_style_attach (widget->style, widget->window);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass      = GDK_INPUT_ONLY;
    attributes.event_mask  = gtk_widget_get_events (widget);
    attributes.event_mask |= (GDK_BUTTON_PRESS_MASK
                              | GDK_BUTTON_RELEASE_MASK
                              | GDK_POINTER_MOTION_MASK
                              | GDK_POINTER_MOTION_HINT_MASK
                              | GDK_ENTER_NOTIFY_MASK
                              | GDK_LEAVE_NOTIFY_MASK
                              | GDK_SCROLL_MASK);
    phat_fan_slider_calc_layout (slider,
                                 &attributes.x,
                                 &attributes.y,
                                 &attributes.width,
                                 &attributes.height);
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    slider->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                           &attributes, attributes_mask);
    gdk_window_set_user_data (slider->event_window, widget);
    gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);

    slider->fan_window = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable (slider->fan_window, TRUE); /* does this even do anything? */
    g_signal_connect (G_OBJECT (slider->fan_window),
                      "expose-event",
                      G_CALLBACK (phat_fan_slider_fan_expose),
                      (gpointer) slider);
    g_signal_connect (G_OBJECT (slider->fan_window),
                      "show",
                      G_CALLBACK (phat_fan_slider_fan_show),
                      (gpointer) slider);

    phat_fan_slider_build_fan_clips (slider);

    slider->hint_window0 = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_realize (slider->hint_window0);
    g_signal_connect (G_OBJECT (slider->hint_window0),
                      "expose-event",
                      G_CALLBACK (phat_fan_slider_hint_expose),
                      (gpointer) slider);

    slider->hint_window1 = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_realize (slider->hint_window1);
    g_signal_connect (G_OBJECT (slider->hint_window1),
                      "expose-event",
                      G_CALLBACK (phat_fan_slider_hint_expose),
                      (gpointer) slider);

    /* a priming call */
    phat_fan_slider_update_hints (slider);
}

static void phat_fan_slider_unrealize (GtkWidget *widget)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
    GtkWidgetClass* klass = GTK_WIDGET_CLASS (parent_class);

    debug ("unrealize\n");
     
    gdk_cursor_unref (slider->arrow_cursor);
    slider->arrow_cursor = NULL;

    gdk_cursor_unref (slider->empty_cursor);
    slider->empty_cursor = NULL;

    gdk_window_set_user_data (slider->event_window, NULL);
    gdk_window_destroy (slider->event_window);
    slider->event_window = NULL;

    gtk_widget_destroy (slider->fan_window);
    slider->fan_window = NULL;

    gdk_region_destroy (slider->fan_clip0);
    slider->fan_clip0 = NULL;

    gdk_region_destroy (slider->fan_clip1);
    slider->fan_clip1 = NULL;

    gtk_widget_destroy (slider->hint_window0);
    slider->hint_window0 = NULL;

    gtk_widget_destroy (slider->hint_window1);
    slider->hint_window1 = NULL;

    gdk_region_destroy (slider->hint_clip0);
    slider->hint_clip0 = NULL;
     
    gdk_region_destroy (slider->hint_clip1);
    slider->hint_clip1 = NULL;
     
    if (klass->unrealize)
        klass->unrealize (widget);
}

  

static void phat_fan_slider_map (GtkWidget *widget)
{
    PhatFanSlider* slider;

    debug ("map\n");
     
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    slider = (PhatFanSlider*) widget;

    gdk_window_show (slider->event_window);

    GTK_WIDGET_CLASS (parent_class)->map (widget);
}

static void phat_fan_slider_unmap (GtkWidget *widget)
{
    PhatFanSlider* slider;

    debug ("unmap\n");
     
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    slider = (PhatFanSlider*) widget;

    gdk_window_hide (slider->event_window);

    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void phat_fan_slider_size_request (GtkWidget*      widget,
                                          GtkRequisition* requisition)
{
    PhatFanSlider* slider;
    int focus_width, focus_pad;
    int pad;
     
    debug ("size request\n");
     
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    slider = (PhatFanSlider*) widget;

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = 2 * (focus_width + focus_pad);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        requisition->width = SLIDER_WIDTH + pad;
        requisition->height = SLIDER_LENGTH + pad;
    }
    else
    {
        requisition->width = SLIDER_LENGTH + pad;
        requisition->height = SLIDER_WIDTH + pad;
    }
}

static void phat_fan_slider_size_allocate (GtkWidget*     widget,
                                           GtkAllocation* allocation)
{
    PhatFanSlider* slider;
    int x, y;
    int w, h;

    debug ("size allocate\n");
     
    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    g_return_if_fail (allocation != NULL);

    slider = (PhatFanSlider*) widget;

    widget->allocation = *allocation;

    phat_fan_slider_calc_layout (slider, &x, &y, &w, &h);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        slider->fan_max_thickness = ((fan_max_height - h)
                                     / (2*FAN_RISE/FAN_RUN));
    }
    else
    {
        slider->fan_max_thickness = ((fan_max_width - w)
                                     / (2*FAN_RISE/FAN_RUN));
    }
     
    if (GTK_WIDGET_REALIZED (widget))
    {
        gdk_window_move_resize (slider->event_window,
                                x, y, w, h);

        phat_fan_slider_build_fan_clips (slider);
    }
}

static gboolean phat_fan_slider_expose (GtkWidget*      widget,
                                        GdkEventExpose* event)
{
    PhatFanSlider* slider;
    int x, y;
    int w, h;
    int fx, fy;                /* "filled" coordinates */
    int fw, fh;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    //debug ("expose\n");
    if (event->count > 0)
        return FALSE;

    slider = (PhatFanSlider*) widget;

    phat_fan_slider_calc_layout (slider, &x, &y, &w, &h);
     
    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (slider->center_val >= 0)
        {
            fw = w;
            fh = ABS (slider->val - slider->center_val) * h;
            fx = x;
            fy = y + h - (slider->center_val * h);

            if ((slider->val > slider->center_val && !slider->inverted)
                || (slider->val < slider->center_val && slider->inverted))
            {
                fy -= fh;
            }

        }
        else
        {
            fw = w;
            fh = slider->val * h;
            fx = x;
            fy = (slider->inverted)? y: y + h - fh;
        }
    }
    else
    {
        if (slider->center_val >= 0)
        {
            fw = ABS (slider->val - slider->center_val) * w;
            fh = h;
            fx = x + (slider->center_val * w);
            fy = y;

            if ((slider->val < slider->center_val && !slider->inverted)
                || (slider->val > slider->center_val && slider->inverted))
            {
                fx -= fw;
            }
        }
        else
        {
            fw = slider->val * w;
            fh = h;
            fx = (slider->inverted)? x + w - fw: x;
            fy = y;
        }
    }
     
    if (!GTK_WIDGET_SENSITIVE (widget))
    {
        gdk_draw_rectangle (widget->window,
                            widget->style->dark_gc[GTK_STATE_INSENSITIVE],
                            TRUE,
                            x, y,
                            w, h);

        gdk_draw_rectangle (widget->window,
                            widget->style->fg_gc[GTK_STATE_INSENSITIVE],
                            TRUE,
                            fx, fy,
                            fw, fh);

        goto skiplines;       /* skip drawing the extra marker lines */
    }
    else
    {
        gdk_draw_rectangle (widget->window,
                            widget->style->dark_gc[GTK_STATE_NORMAL],
                            TRUE,
                            x, y,
                            w, h);

        gdk_draw_rectangle (widget->window,
                            widget->style->base_gc[GTK_STATE_SELECTED],
                            TRUE,
                            fx, fy,
                            fw, fh);
    }

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int line_y;
          
        if (slider->center_val >= 0)
        {
            line_y = fy;

            if ((slider->val > slider->center_val && slider->inverted)
                || (slider->val < slider->center_val && !slider->inverted))
            {
                line_y += fh;
            }
               
            line_y = CLAMP (line_y, y, y + h-1);

            gdk_draw_line (widget->window,
                           widget->style->fg_gc[GTK_STATE_NORMAL],
                           x,
                           line_y,
                           x + w - 1,
                           line_y);

            line_y = (1 - slider->center_val) * h + y;
               
            gdk_draw_line (widget->window,
                           widget->style->base_gc[GTK_STATE_NORMAL],
                           x,
                           line_y,
                           x + w - 1,
                           line_y);
        }
        else
        {
            line_y = fy;

            if (slider->inverted)
                line_y += fh;

            line_y = CLAMP (line_y, y, y + h-1);
          
            gdk_draw_line (widget->window,
                           widget->style->fg_gc[GTK_STATE_NORMAL],
                           x,
                           line_y,
                           x + w - 1,
                           line_y);
        }
    }
    else
    {
        int line_x;
          
        if (slider->center_val >= 0)
        {
            line_x = fx;

            if ((slider->val < slider->center_val && slider->inverted)
                || (slider->val > slider->center_val && !slider->inverted))
            {
                line_x += fw;
            }
               
            line_x = CLAMP (line_x, x, x + w-1);
               
            gdk_draw_line (widget->window,
                           widget->style->fg_gc[GTK_STATE_NORMAL],
                           line_x,
                           y,
                           line_x,
                           y + h - 1);

            line_x =  slider->center_val * w + x;

            gdk_draw_line (widget->window,
                           widget->style->base_gc[GTK_STATE_NORMAL],
                           line_x,
                           y,
                           line_x,
                           y + h - 1);
        }
        else
        {
            line_x = fx;

            if (!slider->inverted)
                line_x += fw;
               
            line_x = CLAMP (line_x, x, x + w-1);
                    
            gdk_draw_line (widget->window,
                           widget->style->fg_gc[GTK_STATE_NORMAL],
                           line_x,
                           y,
                           line_x,
                           y + h - 1);
        }
    }

skiplines:
    gtk_paint_shadow (widget->style,
                      widget->window,
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      NULL, widget, NULL,
                      x, y,
                      w, h);

    if (GTK_WIDGET_HAS_FOCUS (widget))
    {
        int focus_width, focus_pad;
        int pad;
          
        gtk_widget_style_get (widget,
                              "focus-line-width", &focus_width,
                              "focus-padding", &focus_pad,
                              NULL);

        pad = focus_width + focus_pad;

        x -= pad;
        y -= pad;
        w += 2*pad;
        h += 2*pad;

        gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
                         NULL, widget, NULL,
                         x, y, w, h);
    }

    if (GTK_WIDGET_VISIBLE (slider->fan_window))
        gtk_widget_queue_draw (slider->fan_window);
     
    return FALSE;
}

static gboolean phat_fan_slider_button_press (GtkWidget*      widget,
                                              GdkEventButton* event)
{
    PhatFanSlider* slider;
    GtkAllocation* a;
    GtkAllocation* b;
    int width;
    int height;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhatFanSlider*) widget;

    if(event->button == 1)
    {
        gtk_widget_grab_focus (widget);
         
        if (slider->state == STATE_SCROLL)
        {
            slider->state = STATE_NORMAL;
            gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);
            return FALSE;
        }

        gdk_window_set_cursor (slider->event_window, slider->empty_cursor);

        slider->xclick_root = event->x_root;
        slider->xclick = event->x;
        slider->yclick_root = event->y_root;
        slider->yclick = event->y;
        slider->state = STATE_CLICKED;

        gtk_window_present (GTK_WINDOW (slider->hint_window0));
        gtk_window_present (GTK_WINDOW (slider->hint_window1));

        phat_fan_slider_update_hints (slider);

        gdk_window_get_geometry (slider->event_window,
                                 NULL, NULL, &width, &height, NULL);
              
        a = &slider->hint_window0->allocation;
        b = &slider->hint_window1->allocation;

        if (slider->orientation == GTK_ORIENTATION_VERTICAL)
        {
            gtk_window_move (GTK_WINDOW (slider->hint_window0),
                             slider->xclick_root - slider->xclick - a->width,
                             (slider->yclick_root - slider->yclick)
                             + (height - a->height) / 2);

            gtk_window_move (GTK_WINDOW (slider->hint_window1),
                             slider->xclick_root - slider->xclick + width,
                             (slider->yclick_root - slider->yclick)
                             + (height - b->height) / 2);
        }
        else
        {
            gtk_window_move (GTK_WINDOW (slider->hint_window0),
                             (slider->xclick_root - slider->xclick)
                             + (width - a->width) / 2,
                             slider->yclick_root - slider->yclick - a->height);

            gtk_window_move (GTK_WINDOW (slider->hint_window1),
                             (slider->xclick_root - slider->xclick)
                             + (width - b->width) / 2,
                             slider->yclick_root - slider->yclick + height);
        }
    }
    else if (event->button == 2 && slider->use_default_value)
    {
        /* reset to default value */
        phat_fan_slider_set_value(slider, slider->default_value);
        return TRUE;
    }

    return FALSE;
}
    
static gboolean phat_fan_slider_button_release (GtkWidget*      widget,
                                                GdkEventButton* event)
{
    PhatFanSlider* slider;
     
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhatFanSlider*) widget;
    gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);

    if (slider->state == STATE_CLICKED)
    {
        slider->state = STATE_NORMAL;

        phat_warp_pointer (event->x_root,
                           event->y_root,
                           slider->xclick_root,
                           slider->yclick_root);

        if (GTK_WIDGET_VISIBLE (slider->fan_window))
            gtk_widget_hide (slider->fan_window);

        if (GTK_WIDGET_VISIBLE (slider->hint_window0))
            gtk_widget_hide (slider->hint_window0);

        if (GTK_WIDGET_VISIBLE (slider->hint_window1))
            gtk_widget_hide (slider->hint_window1);
    }
     
    return FALSE;
}

static gboolean phat_fan_slider_key_press (GtkWidget* widget,
                                           GdkEventKey* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
    GtkAdjustment* adj = slider->adjustment_prv;
    double inc;

    debug ("key press\n");

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        switch (event->keyval)
        {
        case GDK_Up:
            inc = adj->step_increment;
            break;
        case GDK_Down:
            inc = -adj->step_increment;
            break;
        case GDK_Page_Up:
            inc = adj->page_increment;
            break;
        case GDK_Page_Down:
            inc = -adj->page_increment;
            break;
        default:
            return FALSE;
        }
    }
    else
    {
        switch (event->keyval)
        {
        case GDK_Right:
            inc = adj->step_increment;
            break;
        case GDK_Left:
            inc = -adj->step_increment;
            break;
        case GDK_Page_Up:
            inc = adj->page_increment;
            break;
        case GDK_Page_Down:
            inc = -adj->page_increment;
            break;
        default:
            return FALSE;
        }
    }

    if (slider->inverted)
        inc = -inc;
     
    gtk_adjustment_set_value (adj, adj->value + inc);

    return TRUE;
}

static gboolean phat_fan_slider_scroll (GtkWidget* widget,
                                        GdkEventScroll* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);


    gtk_widget_grab_focus (widget);
     
    slider->state = STATE_SCROLL;

    slider->xclick_root = event->x_root;
    slider->yclick_root = event->y_root;
    slider->xclick = event->x;
    slider->yclick = event->y;

    gdk_window_set_cursor (slider->event_window, slider->empty_cursor);
     
    if (((event->direction == GDK_SCROLL_UP
          || event->direction == GDK_SCROLL_RIGHT) && !slider->inverted)

        || ((event->direction == GDK_SCROLL_DOWN
             || event->direction == GDK_SCROLL_LEFT) && slider->inverted))
    {
        gtk_adjustment_set_value (slider->adjustment_prv,
                                  (slider->adjustment_prv->value
                                   + slider->adjustment_prv->page_increment));
    }
    else
    {
        gtk_adjustment_set_value (slider->adjustment_prv,
                                  (slider->adjustment_prv->value
                                   - slider->adjustment_prv->page_increment));
    }

    return TRUE;
}

/* ctrl locks precision, shift locks value */
static gboolean phat_fan_slider_motion_notify (GtkWidget*      widget,
                                               GdkEventMotion* event)
{
    PhatFanSlider* slider;
 
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhatFanSlider*) widget;

    switch (slider->state)
    {
    case STATE_SCROLL:
        if (ABS (event->x - slider->xclick) >= THRESHOLD
            || ABS (event->y - slider->yclick) >= THRESHOLD)
        {
            gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);
            slider->state = STATE_NORMAL;
        }
    case STATE_NORMAL:
        goto skip;
        break;
    }
        
    if (!(event->state & GDK_CONTROL_MASK))
        phat_fan_slider_update_fan (slider, event->x, event->y);

    if (!(event->state & GDK_SHIFT_MASK))
        phat_fan_slider_update_value (slider, event->x_root, event->y_root);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int destx = event->x_root;
        int width;

        gdk_window_get_geometry (slider->event_window,
                                 NULL, NULL, &width, NULL, NULL);

        if (GTK_WIDGET_VISIBLE (slider->fan_window))
        {
            if (event->x_root > slider->xclick_root)
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    destx = (slider->xclick_root
                             + width
                             - slider->xclick
                             + slider->fan_window->allocation.width);
                }
                else
                {
                    destx = MIN (event->x_root,
                                 slider->xclick_root
                                 + width
                                 - slider->xclick
                                 + slider->fan_window->allocation.width);
                }
            }
            else
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    destx = (slider->xclick_root
                             - slider->xclick
                             - slider->fan_window->allocation.width);
                }
                else
                {
                    destx = MAX (event->x_root,
                                 slider->xclick_root
                                 - slider->xclick
                                 - slider->fan_window->allocation.width);
                }
            }
        }
        else if (event->state & GDK_CONTROL_MASK)
        {
            destx = slider->xclick_root;
        }

        phat_warp_pointer (event->x_root,
                           event->y_root,
                           destx,
                           slider->yclick_root);
    }
    else
    {
        int desty = event->y_root;
        int height;

        gdk_window_get_geometry (slider->event_window,
                                 NULL, NULL, NULL, &height, NULL);
               
        if (GTK_WIDGET_VISIBLE (slider->fan_window))
        {
            if (event->y_root > slider->yclick_root)
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    desty = (slider->yclick_root
                             + height
                             - slider->yclick
                             + slider->fan_window->allocation.height);
                }
                else
                {
                    desty = MIN (event->y_root,
                                 slider->yclick_root
                                 + height
                                 - slider->yclick
                                 + slider->fan_window->allocation.height);
                }
            }
            else
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    desty = (slider->yclick_root
                             - slider->yclick
                             - slider->fan_window->allocation.height);
                }
                else
                {
                    desty = MAX (event->y_root,
                                 slider->yclick_root
                                 - slider->yclick
                                 - slider->fan_window->allocation.height);
                }
            }
        }
        else if (event->state & GDK_CONTROL_MASK)
        {
            desty = slider->yclick_root;
        }
               
        phat_warp_pointer (event->x_root,
                           event->y_root,
                           slider->xclick_root,
                           desty);
    }
          
    gtk_widget_queue_draw (widget);

    /* necessary in case update_fan() doesn't get called */
    if (GTK_WIDGET_VISIBLE (slider->fan_window))
        gtk_widget_queue_draw (slider->fan_window);

skip:
    /* signal that we want more motion events */
    gdk_window_get_pointer (NULL, NULL, NULL, NULL);
     
    return FALSE;
}


static gboolean phat_fan_slider_enter_notify (GtkWidget* widget,
                                              GdkEventCrossing* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
     
    if (slider->state == STATE_NORMAL)
        gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);

    return FALSE;
}

static gboolean phat_fan_slider_leave_notify (GtkWidget* widget,
                                              GdkEventCrossing* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
     
    if (slider->state == STATE_SCROLL)
    {
        gdk_window_set_cursor (slider->event_window, NULL);
        slider->state = STATE_NORMAL;
    }

    return FALSE;
}

static void phat_fan_slider_draw_fan (PhatFanSlider* slider)
{
    GtkWidget* widget = GTK_WIDGET (slider);
    GtkWidget* window = slider->fan_window;
    GdkPoint points[4];
    int x, y;
    int start, stop;
    int length;
    float value;
    int center_start;
    int center_stop;
     
    if (!GTK_WIDGET_DRAWABLE (slider->fan_window))
        return;
     
    gtk_window_get_position (GTK_WINDOW (slider->fan_window),
                             &x, &y);

    length = phat_fan_slider_get_fan_length (slider);

    gdk_window_clear (slider->fan_window->window);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (!slider->inverted)
            value = 1.0 - slider->val;
        else
            value = slider->val;

        start = ((value * widget->allocation.height)
                 + (window->allocation.height
                    - widget->allocation.height) / 2);

        stop = ((value * length)
                + (window->allocation.height
                   - length) / 2);
          
        if (slider->center_val >= 0)
        {
            center_start = (widget->allocation.height * (1 - slider->center_val)
                            + (window->allocation.height
                               - widget->allocation.height) / 2);

            center_stop = length * (1 - slider->center_val) + (fan_max_height - length)/2;
        }
        else
        {
            if (!slider->inverted)
                center_start = center_stop = window->allocation.height;
            else
                center_start = center_stop = 0;
        }
               
        if (x > slider->xclick_root)
        {
            points[0].x = 0;
            points[0].y = start;
            points[1].x = 0;
            points[1].y = center_start;
            points[2].x = window->allocation.width;
            points[2].y = center_stop;
            points[3].x = window->allocation.width;
            points[3].y = stop;
            gdk_draw_polygon (window->window, widget->style->base_gc[GTK_STATE_SELECTED],
                              TRUE, points, 4);
     
            gdk_draw_line (window->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                           0, start,
                           window->allocation.width, stop);

            if (slider->center_val >= 0)
                gdk_draw_line (window->window, widget->style->base_gc[GTK_STATE_NORMAL],
                               0, center_start,
                               window->allocation.width, center_stop);
        }
        else
        {
            points[0].x = window->allocation.width;
            points[0].y = start;
            points[1].x = window->allocation.width;
            points[1].y = center_start;
            points[2].x = 0;
            points[2].y = center_stop;
            points[3].x = 0;
            points[3].y = stop;
            gdk_draw_polygon (window->window, widget->style->base_gc[GTK_STATE_SELECTED],
                              TRUE, points, 4);
     
            gdk_draw_line (window->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                           window->allocation.width, start,
                           0, stop);

            if (slider->center_val >= 0)
                gdk_draw_line (window->window, widget->style->base_gc[GTK_STATE_NORMAL],
                               0, center_stop,
                               window->allocation.width, center_start);
        }
    }
    else
    {
        if (!slider->inverted)
            value = slider->val;
        else
            value = 1.0 - slider->val;

        start = ((value * widget->allocation.width)
                 + (window->allocation.width
                    - widget->allocation.width) / 2);

        stop = ((value * length)
                + (window->allocation.width
                   - length) / 2);
          
        if (slider->center_val >= 0)
        {
            center_start = (widget->allocation.width * slider->center_val
                            + (window->allocation.width
                               - widget->allocation.width) / 2);

            center_stop = length * slider->center_val + (fan_max_width - length)/2;
        }
        else
        {
            if (!slider->inverted)
                center_start = center_stop = 0;
            else
                center_start = center_stop = window->allocation.width;
        }
          
        if (y > slider->yclick_root)
        {
            points[0].x = start;
            points[0].y = 0;
            points[1].x = stop;
            points[1].y = window->allocation.height;
            points[2].x = center_stop;
            points[2].y = window->allocation.height;
            points[3].x = center_start;
            points[3].y = 0;
            gdk_draw_polygon (window->window, widget->style->base_gc[GTK_STATE_SELECTED],
                              TRUE, points, 4);
     
            gdk_draw_line (window->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                           start, 0,
                           stop, window->allocation.height);

            if (slider->center_val >= 0)
                gdk_draw_line (window->window, widget->style->base_gc[GTK_STATE_NORMAL],
                               center_stop, window->allocation.height,
                               center_start, 0);
        }
        else
        {
            points[0].x = start;
            points[0].y = window->allocation.height;
            points[1].x = stop;
            points[1].y = 0;
            points[2].x = center_stop;
            points[2].y = 0;
            points[3].x = center_start;
            points[3].y = window->allocation.height;
            gdk_draw_polygon (window->window, widget->style->base_gc[GTK_STATE_SELECTED],
                              TRUE, points, 4);
     
            gdk_draw_line (window->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                           start, window->allocation.height,
                           stop, 0);

            if (slider->center_val >= 0)
                gdk_draw_line (window->window, widget->style->base_gc[GTK_STATE_NORMAL],
                               center_start, window->allocation.height,
                               center_stop, 0);
        }
    }
}

static void phat_fan_slider_calc_layout (PhatFanSlider* slider,
                                         int* x, int* y, int* w, int* h)
{
    GtkWidget* widget = GTK_WIDGET (slider);
    int focus_width, focus_pad;
    int pad;
          
    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = focus_width + focus_pad;

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        *x = widget->allocation.x + (widget->allocation.width - SLIDER_WIDTH) / 2;
        *y = widget->allocation.y + pad;
        *w = SLIDER_WIDTH;
        *h = widget->allocation.height - 2*pad;
    }
    else
    {
        *x = widget->allocation.x + pad;
        *y = widget->allocation.y + (widget->allocation.height - SLIDER_WIDTH) / 2;
        *w = widget->allocation.width - 2*pad;
        *h = SLIDER_WIDTH;
    }
}

static void phat_fan_slider_update_value (PhatFanSlider* slider,
                                          int x_root, int y_root)
{
    int length;
    double oldval;
    double value;
    double inc;
     
    if (slider->state != STATE_CLICKED)
        return;

    oldval = slider->val;

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (GTK_WIDGET_DRAWABLE (slider->fan_window)
            && (x_root != slider->xclick_root))
        {
            length = phat_fan_slider_get_fan_length (slider);
            inc = ((slider->yclick_root - y_root)
                   * 1.0 / length);
        }
        else
        {
            inc = ((slider->yclick_root - y_root)
                   * 1.0 / GTK_WIDGET (slider)->allocation.height);
        }
    }
    else
    {
        if (GTK_WIDGET_DRAWABLE (slider->fan_window)
            && (y_root != slider->yclick_root))
        {
            length = phat_fan_slider_get_fan_length (slider);
            inc = ((x_root - slider->xclick_root)
                   * 1.0 / length);
        }
        else
        {
            inc = ((x_root - slider->xclick_root)
                   * 1.0 / GTK_WIDGET (slider)->allocation.width);
        }
    }

    if (slider->inverted)
        inc = -inc;

    slider->val += inc;
    slider->val = CLAMP (slider->val, 0, 1);

    if (slider->val != oldval)
    {
        value = (slider->adjustment_prv->lower * (1.0 - slider->val)
                 + slider->adjustment_prv->upper * slider->val);

        g_signal_handlers_block_by_func (G_OBJECT (slider),
                                         phat_fan_slider_adjustment_value_changed,
                                         (gpointer) slider);

        gtk_adjustment_set_value (slider->adjustment_prv, value);
          
        g_signal_emit (G_OBJECT (slider),
                       phat_fan_slider_signals[VALUE_CHANGED_SIGNAL], 0);

        g_signal_handlers_unblock_by_func (G_OBJECT (slider),
                                           phat_fan_slider_adjustment_value_changed,
                                           (gpointer) slider);
    }
}

static void phat_fan_slider_update_fan (PhatFanSlider* slider,
                                        int x, int y)
{
    int width;
    int height;
    int fanx, fany;
    int w, h;

    if (slider->state != STATE_CLICKED)
        return;

    gdk_window_get_geometry (slider->event_window,
                             NULL, NULL, &w, &h, NULL);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (x > w)
        {
            width = x - w;
            width = CLAMP (width, 0, slider->fan_max_thickness);
          
            gtk_window_resize (GTK_WINDOW (slider->fan_window), width, fan_max_height);

            fanx = slider->xclick_root + (w - slider->xclick);

            fany = (slider->yclick_root - slider->yclick)
                - ((fan_max_height - h) / 2);

            gtk_window_move (GTK_WINDOW (slider->fan_window), fanx, fany);          

            if (!GTK_WIDGET_VISIBLE (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            gdk_window_shape_combine_region (slider->fan_window->window,
                                             slider->fan_clip1,
                                             0, 0);

            if (GTK_WIDGET_VISIBLE (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);
            if (GTK_WIDGET_VISIBLE (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (x < 0)
        {
            width = -x;
            width = CLAMP (width, 0, slider->fan_max_thickness);

            gtk_window_resize (GTK_WINDOW (slider->fan_window), width, fan_max_height);

            fanx = slider->xclick_root - slider->xclick
                - slider->fan_window->allocation.width;

            fany = (slider->yclick_root - slider->yclick)
                - ((fan_max_height - h) / 2);

            gtk_window_move (GTK_WINDOW (slider->fan_window), fanx, fany);

            if (!GTK_WIDGET_VISIBLE (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            gdk_window_shape_combine_region (slider->fan_window->window,
                                             slider->fan_clip0,
                                             slider->fan_window->allocation.width
                                             - slider->fan_max_thickness,
                                             0);

            if (GTK_WIDGET_VISIBLE (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);
            if (GTK_WIDGET_VISIBLE (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (GTK_WIDGET_VISIBLE (slider->fan_window))
        {
            gtk_widget_hide (slider->fan_window);
        }
    }
    else
    {
        if (y > h)
        {
            height = y - h;
            height = CLAMP (height, 0, slider->fan_max_thickness);
          
            gtk_window_resize (GTK_WINDOW (slider->fan_window), fan_max_width, height);

            fanx = (slider->xclick_root - slider->xclick)
                - ((fan_max_width - w) / 2);

            fany = slider->yclick_root + (h - slider->yclick);

            gtk_window_move (GTK_WINDOW (slider->fan_window), fanx, fany);          

            if (!GTK_WIDGET_VISIBLE (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            gdk_window_shape_combine_region (slider->fan_window->window,
                                             slider->fan_clip1, 0, 0);

            if (GTK_WIDGET_VISIBLE (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);
            if (GTK_WIDGET_VISIBLE (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (y < 0)
        {
            height = -y;
            height = CLAMP (height, 0, slider->fan_max_thickness);

            gtk_window_resize (GTK_WINDOW (slider->fan_window), fan_max_width, height);

            fanx = (slider->xclick_root - slider->xclick)
                - ((fan_max_width - w) / 2);

            fany = slider->yclick_root - slider->yclick
                - slider->fan_window->allocation.height;

            gtk_window_move (GTK_WINDOW (slider->fan_window), fanx, fany);

            if (!GTK_WIDGET_VISIBLE (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            gdk_window_shape_combine_region (slider->fan_window->window,
                                             slider->fan_clip0,
                                             0,
                                             slider->fan_window->allocation.height
                                             - slider->fan_max_thickness);
               
            if (GTK_WIDGET_VISIBLE (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);
            if (GTK_WIDGET_VISIBLE (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (GTK_WIDGET_VISIBLE (slider->fan_window))
        {
            gtk_widget_hide (slider->fan_window);
        }
    }
}

static int phat_fan_slider_get_fan_length (PhatFanSlider* slider)
{
    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        return 2 * (FAN_RISE / FAN_RUN)
            * slider->fan_window->allocation.width
            + GTK_WIDGET (slider)->allocation.height;
    }
    else
    {
        return 2 * (FAN_RISE / FAN_RUN)
            * slider->fan_window->allocation.height
            + GTK_WIDGET (slider)->allocation.width;
    }
}

static gboolean phat_fan_slider_fan_expose (GtkWidget*      widget,
                                            GdkEventExpose* event,
                                            PhatFanSlider*  slider)
{
    phat_fan_slider_draw_fan (slider);

    return TRUE;
}

static void phat_fan_slider_fan_show (GtkWidget* widget,
                                      GtkWidget* slider)
{
    gdk_window_set_background (widget->window, &slider->style->dark[GTK_STATE_NORMAL]);
}

static gboolean phat_fan_slider_hint_expose (GtkWidget* widget,
                                             GdkEventExpose* event,
                                             GtkWidget* slider)
{
    gdk_draw_rectangle (widget->window,
                        slider->style->fg_gc[GTK_STATE_NORMAL],
                        TRUE,
                        0, 0,
                        widget->allocation.width,
                        widget->allocation.height);

    return TRUE;
}

static void phat_fan_slider_build_fan_clips (PhatFanSlider* slider)
{
    GtkWidget* widget = GTK_WIDGET (slider);
    int max_thickness = slider->fan_max_thickness;
     
    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        GdkPoint points0[4] = {
            { max_thickness, (fan_max_height - widget->allocation.height) / 2 },
            { 0, 0 },
            { 0, fan_max_height },
            { max_thickness, fan_max_height - (fan_max_height - widget->allocation.height)/2 }
        };

        GdkPoint points1[4] = {
            { 0, (fan_max_height - widget->allocation.height) / 2 },
            { max_thickness, 0 },
            { max_thickness, fan_max_height },
            { 0, fan_max_height - (fan_max_height - widget->allocation.height) / 2 }
        };

        GdkRegion* oldfanclip0 = slider->fan_clip0;
        GdkRegion* oldfanclip1 = slider->fan_clip1;
          
        slider->fan_clip0 = gdk_region_polygon (points0, 4, GDK_EVEN_ODD_RULE);
        slider->fan_clip1 = gdk_region_polygon (points1, 4, GDK_EVEN_ODD_RULE);

        if (oldfanclip0 != NULL)
            gdk_region_destroy (oldfanclip0);
        if (oldfanclip1 != NULL)
            gdk_region_destroy (oldfanclip1);
    }
    else
    {
        GdkPoint points0[4] = {
            { (fan_max_width - widget->allocation.width) / 2, max_thickness },
            { 0, 0 },
            { fan_max_width, 0 },
            { fan_max_width - ((fan_max_width - widget->allocation.width) / 2), max_thickness }
        };
          
        GdkPoint points1[4] = {
            { (fan_max_width - widget->allocation.width) / 2, 0 },
            { 0, max_thickness },
            { fan_max_width, max_thickness },
            { fan_max_width - (fan_max_width - widget->allocation.width) / 2, 0 }
        };

        GdkRegion* oldfanclip0 = slider->fan_clip0;
        GdkRegion* oldfanclip1 = slider->fan_clip1;
          
        slider->fan_clip0 = gdk_region_polygon (points0, 4, GDK_EVEN_ODD_RULE);
        slider->fan_clip1 = gdk_region_polygon (points1, 4, GDK_EVEN_ODD_RULE);

        if (oldfanclip0 != NULL)
            gdk_region_destroy (oldfanclip0);
        if (oldfanclip1 != NULL)
            gdk_region_destroy (oldfanclip1);
    }
}

/* setup 9x9 hint arrows; I had to do a lot of juggling to get things
 * to look right, and I have no clue why I ended up needing to write
 * the code as follows */
static void phat_fan_slider_update_hints (PhatFanSlider* slider)
{
    GdkRegion* oldclip0 = slider->hint_clip0;
    GdkRegion* oldclip1 = slider->hint_clip1;

    gtk_window_resize (GTK_WINDOW (slider->hint_window0), 9, 9);
    gtk_window_resize (GTK_WINDOW (slider->hint_window1), 9, 9);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        GdkPoint points0[7] = {
            { 8, 3 },
            { 4, 3 },
            { 4, 0 },
            { 0, 4 },
            { 4, 8 },
            { 4, 6 },
            { 8, 6 }
        };

        GdkPoint points1[7] = {
            { 0, 3 },
            { 4, 3 },
            { 4, 0 },
            { 8, 4 },
            { 4, 8 },
            { 4, 6 },
            { 0, 6 }
        };

        slider->hint_clip0 = gdk_region_polygon (points0, 7, GDK_EVEN_ODD_RULE);
        slider->hint_clip1 = gdk_region_polygon (points1, 7, GDK_EVEN_ODD_RULE);

        gdk_window_shape_combine_region (slider->hint_window0->window,
                                         slider->hint_clip0, 0, 0);
        gdk_window_shape_combine_region (slider->hint_window1->window,
                                         slider->hint_clip1, 0, 0);
    }
    else
    {
        GdkPoint points0[7] = {
            { 3, 8 },
            { 3, 4 },
            { 0, 4 },
            { 4,-1 },
            { 9, 4 },
            { 6, 4 },
            { 6, 8 }
        };

        GdkPoint points1[7] = {
            { 3, 0 },
            { 3, 4 },
            { 0, 4 },
            { 4, 9 },
            { 9, 4 },
            { 6, 4 },
            { 6, 0 }
        };

        slider->hint_clip0 = gdk_region_polygon (points0, 7, GDK_EVEN_ODD_RULE);
        slider->hint_clip1 = gdk_region_polygon (points1, 7, GDK_EVEN_ODD_RULE);

        gdk_window_shape_combine_region (slider->hint_window0->window,
                                         slider->hint_clip0, 0, 0);
        gdk_window_shape_combine_region (slider->hint_window1->window,
                                         slider->hint_clip1, 0, 0);
    }
          
    if (oldclip0 != NULL)
        gdk_region_destroy (oldclip0);
    if (oldclip1 != NULL)
        gdk_region_destroy (oldclip1);
}

static void phat_fan_slider_adjustment_changed (GtkAdjustment* adjustment,
                                                PhatFanSlider* slider)
{
    GtkWidget* widget;

    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    widget = GTK_WIDGET (slider);

    if (adjustment->lower < 0 && adjustment->upper > 0)
    {
        slider->center_val = (-adjustment->lower
                              / (adjustment->upper - adjustment->lower));
    }
    else
    {
        slider->center_val = -1;
    }

    slider->val = ((adjustment->value - adjustment->lower)
                   / (adjustment->upper - adjustment->lower));
     
    gtk_widget_queue_draw (GTK_WIDGET (slider));

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_process_updates (widget->window, FALSE);

    g_signal_emit (G_OBJECT (slider),
                   phat_fan_slider_signals[CHANGED_SIGNAL], 0);
}

static void phat_fan_slider_adjustment_value_changed (GtkAdjustment* adjustment,
                                                      PhatFanSlider* slider)
{
    GtkWidget* widget;

    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    widget = GTK_WIDGET (slider);

    slider->val = ((adjustment->value - adjustment->lower)
                   / (adjustment->upper - adjustment->lower));

    gtk_widget_queue_draw (widget);

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_process_updates (widget->window, FALSE);

    g_signal_emit (G_OBJECT (slider),
                   phat_fan_slider_signals[VALUE_CHANGED_SIGNAL], 0);

    if (slider->adjustment != NULL)
    {
        phat_fan_slider_get_value(slider); /* update value of external adjustment */
    }
}
