/* 
 *
 * Most of this code comes from gAlan 0.2.0, copyright (C) 1999
 * Tony Garnock-Jones, with modifications by Sean Bolton,
 * copyright (c) 2004.  (gtkdial.c rolls over in its grave.)
 *
 * Phatised by Loki Davison.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <math.h>
#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "phatknob.h"

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif

#define SCROLL_DELAY_LENGTH     300
#define KNOB_SIZE               50

enum
{
    STATE_IDLE,         
    STATE_PRESSED,              
    STATE_DRAGGING,             
};

/* signals */
enum
{
    VALUE_CHANGED_SIGNAL,
    CHANGED_SIGNAL,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];
static void phat_knob_class_init(PhatKnobClass *klass);
static void phat_knob_init(PhatKnob *knob);
static void phat_knob_destroy(GtkObject *object);
static void phat_knob_realize(GtkWidget *widget);
static void phat_knob_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void phat_knob_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static gint phat_knob_expose(GtkWidget *widget, GdkEventExpose *event);
static gint phat_knob_button_press(GtkWidget *widget, GdkEventButton *event);
static gint phat_knob_button_release(GtkWidget *widget, GdkEventButton *event);
static gint phat_knob_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gint phat_knob_timer(PhatKnob *knob);

static void phat_knob_update_mouse_update(PhatKnob *knob);
static void phat_knob_update_mouse(PhatKnob *knob, gint x, gint y, gboolean absolute);
static void phat_knob_update(PhatKnob *knob);
static void phat_knob_adjustment_changed(GtkAdjustment *adjustment, gpointer data);
static void phat_knob_adjustment_value_changed(GtkAdjustment *adjustment, gpointer data);

static void phat_knob_external_adjustment_changed(GtkAdjustment *adjustment, gpointer data);
static void phat_knob_external_adjustment_value_changed(GtkAdjustment *adjustment, gpointer data);

static void phat_knob_update_internal_adjustment(PhatKnob * knob);

GError *gerror;

/* Local data */

static GtkWidgetClass *parent_class = NULL;

GType phat_knob_get_type(void) 
{
    static GType knob_type = 0;

    if (!knob_type) {
        static const GTypeInfo info = 
            {
                sizeof (PhatKnobClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_knob_class_init,
                NULL,
                NULL,
                sizeof (PhatKnob),
                0,
                (GInstanceInitFunc) phat_knob_init,
            };

        knob_type =  g_type_register_static (GTK_TYPE_WIDGET,
                                             "PhatKnob",
                                             &info,
                                             0);
    }

    return knob_type;
}

static void phat_knob_class_init (PhatKnobClass *class) {
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass*) class;
    widget_class = (GtkWidgetClass*) class;

    parent_class = gtk_type_class(gtk_widget_get_type());

    object_class->destroy = phat_knob_destroy;

    widget_class->realize = phat_knob_realize;
    widget_class->expose_event = phat_knob_expose;
    widget_class->size_request = phat_knob_size_request;
    widget_class->size_allocate = phat_knob_size_allocate;
    widget_class->button_press_event = phat_knob_button_press;
    widget_class->button_release_event = phat_knob_button_release;
    widget_class->motion_notify_event = phat_knob_motion_notify;

    /**
     * PhatKnob::value-changed:
     * @knob: the object on which the signal was emitted
     *
     * The "value-changed" signal is emitted when the value of the
     * knobbutton's adjustment changes.
     *
     */
    signals[VALUE_CHANGED_SIGNAL] =
        g_signal_new ("value-changed",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhatKnobClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    class->value_changed = NULL;
}

static void phat_knob_init (PhatKnob *knob) {
    knob->policy = GTK_UPDATE_CONTINUOUS;
    knob->state = STATE_IDLE;
    knob->saved_x = knob->saved_y = 0;
    knob->timer = 0;
    knob->pixbuf = NULL;
    knob->mask = NULL;
    knob->mask_gc = NULL;
    knob->red_gc = NULL;
    knob->old_value = 0.0;
    knob->old_lower = 0.0;
    knob->old_upper = 0.0;
    knob->is_log = 0;
    knob->adjustment = NULL;
    knob->adjustment_prv = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.1, 0.0);
    phat_knob_set_adjustment(knob, knob->adjustment_prv);
}

GtkWidget *phat_knob_new(GtkAdjustment *adjustment) {
    PhatKnob *knob;

    knob = gtk_type_new(phat_knob_get_type());

    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    knob->adjustment = adjustment;

    gtk_signal_connect(
        GTK_OBJECT(adjustment),
        "changed",
        GTK_SIGNAL_FUNC(phat_knob_external_adjustment_changed),
        (gpointer)knob);
    gtk_signal_connect(
        GTK_OBJECT(adjustment),
        "value_changed",
        GTK_SIGNAL_FUNC(phat_knob_external_adjustment_value_changed),
        (gpointer)knob);


    // correlate internal adj to externally visable one
    phat_knob_update_internal_adjustment(knob);
    
    return GTK_WIDGET(knob);
}

/**
 * phat_knob_new_with_range:
 * @value: the initial value the new knob should have
 * @lower: the lowest value the new knob will allow
 * @upper: the highest value the new knob will allow
 * @step: increment added or subtracted when turning
 *
 * Creates a new #PhatKnob.  The knob will create a new
 * #GtkAdjustment from @value, @lower, @upper, and @step.  If these
 * parameters represent a bogus configuration, the program will
 * terminate.
 *
 * Returns: a newly created #PhatKnob
 * 
 */
GtkWidget* phat_knob_new_with_range (double value, double lower,
                                     double upper, double step)
{
    GtkAdjustment* adj;

    adj = (GtkAdjustment*) gtk_adjustment_new (value, lower, upper, step, step, 0);

    return phat_knob_new (adj);
}

/* Update internal adjustment from external one */
static void
phat_knob_update_internal_adjustment(
    PhatKnob * knob)
{
    if (knob->is_log)
    {   
        gtk_adjustment_set_value(
            (GtkAdjustment *)knob->adjustment_prv,
            log(knob->adjustment->value - knob->adjustment->lower) / log(knob->adjustment->upper - knob->adjustment->lower));
    }
    else
    {
        gtk_adjustment_set_value(
            (GtkAdjustment *)knob->adjustment_prv,
            (knob->adjustment->value - knob->adjustment->lower) / (knob->adjustment->upper - knob->adjustment->lower));
    }
}

/**
 * phat_knob_set_value:
 * @knob: a #PhatKnob
 * @value: a new value for the knob
 * 
 * Sets the current value of the knob.  If the value is outside the
 * range of values allowed by @knob, it will be clamped.  The knob
 * emits the "value-changed" signal if the value changes.
 *
 */
void phat_knob_set_value (PhatKnob* knob, double value)
{
    g_return_if_fail (PHAT_IS_KNOB (knob));

    value = CLAMP (value,
                   knob->adjustment->lower,
                   knob->adjustment->upper);

    gtk_adjustment_set_value (knob->adjustment, value);

    phat_knob_update_internal_adjustment(knob);
}

/**
 * phat_knob_get_value:
 * @knob: a #PhatKnob
 *
 * Retrieves the current value of the knob.
 *
 * Returns: current value of the knob
 *
 */
double phat_knob_get_value (PhatKnob* knob)
{
    g_return_val_if_fail (PHAT_IS_KNOB (knob), 0);

    if(knob->is_log)
    {
        gtk_adjustment_set_value((GtkAdjustment *)knob->adjustment, exp((knob->adjustment_prv->value) * 
                                                                        (log(knob->adjustment->upper - knob->adjustment->lower))) + knob->adjustment->lower);
        //printf("setting prv val %f lower %f upper %f \n", knob->adjustment_prv->value, knob->adjustment->lower, knob->adjustment->upper);
    }
    else
    {
        gtk_adjustment_set_value((GtkAdjustment *)knob->adjustment, (knob->adjustment_prv->value * (knob->adjustment->upper - knob->adjustment->lower) + knob->adjustment->lower));
    }

    return knob->adjustment->value;
}

/**
 * phat_knob_set_range:
 * @knob: a #PhatKnob
 * @lower: lowest allowable value
 * @upper: highest allowable value
 * 
 * Sets the range of allowable values for the knob, and clamps the
 * knob's current value to be between @lower and @upper.
 */
void phat_knob_set_range (PhatKnob* knob,
                          double lower, double upper)
{
    double value;

    g_return_if_fail (PHAT_IS_KNOB (knob));
    g_return_if_fail (lower <= upper);

    knob->adjustment->lower = lower;
    knob->adjustment->upper = upper;

    value = CLAMP (knob->adjustment->value,
                   knob->adjustment->lower,
                   knob->adjustment->upper);

    gtk_adjustment_changed (knob->adjustment);
    gtk_adjustment_set_value (knob->adjustment, value);
}

void phat_knob_set_log (PhatKnob* knob, gboolean is_log)
{
    g_return_if_fail (PHAT_IS_KNOB (knob));

    knob->is_log = is_log;
}

gboolean phat_knob_is_log (PhatKnob* knob)
{
    g_return_val_if_fail (PHAT_IS_KNOB (knob), 0);

    return knob->is_log;
}

static void phat_knob_destroy(GtkObject *object) {
    PhatKnob *knob;

    g_return_if_fail(object != NULL);
    g_return_if_fail(PHAT_IS_KNOB(object));

    knob = PHAT_KNOB(object);

    if (knob->adjustment) {
        gtk_object_destroy(GTK_OBJECT(knob->adjustment));
        knob->adjustment = NULL;
    }

    if (knob->adjustment_prv)
    {
        //didn't call ref on this one so just destroy
        gtk_object_destroy ((GtkObject*)knob->adjustment_prv);
        knob->adjustment_prv = NULL;
    }

    if (knob->pixbuf) {
        gdk_pixbuf_unref(knob->pixbuf);
        knob->pixbuf = NULL;
    }

    if (knob->mask) {
        gdk_bitmap_unref(knob->mask);
        knob->mask = NULL;
    }

    if (knob->mask_gc) {
        gdk_gc_unref(knob->mask_gc);
        knob->mask_gc = NULL;
    }
    if (knob->red_gc) {
        gdk_gc_unref(knob->red_gc);
        knob->red_gc = NULL;
    }

    if (GTK_OBJECT_CLASS(parent_class)->destroy)
        (*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

GtkAdjustment* phat_knob_get_adjustment(PhatKnob *knob) {
    g_return_val_if_fail(knob != NULL, NULL);
    g_return_val_if_fail(PHAT_IS_KNOB(knob), NULL);

    return knob->adjustment;
}

void phat_knob_set_update_policy(PhatKnob *knob, GtkUpdateType policy) {
    g_return_if_fail (knob != NULL);
    g_return_if_fail (PHAT_IS_KNOB (knob));

    knob->policy = policy;
}

void phat_knob_set_adjustment(PhatKnob *knob, GtkAdjustment *adjustment) {
    g_return_if_fail (knob != NULL);
    g_return_if_fail (PHAT_IS_KNOB (knob));

    gtk_signal_connect(GTK_OBJECT(adjustment), "changed",
                       GTK_SIGNAL_FUNC(phat_knob_adjustment_changed), (gpointer) knob);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
                       GTK_SIGNAL_FUNC(phat_knob_adjustment_value_changed), (gpointer) knob);

    knob->old_value = adjustment->value;
    knob->old_lower = adjustment->lower;
    knob->old_upper = adjustment->upper;

    phat_knob_update(knob);
}

static void phat_knob_realize(GtkWidget *widget) {
    PhatKnob *knob;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GdkColor color = { 0, 0xffff, 0, 0 };

    g_return_if_fail(widget != NULL);
    g_return_if_fail(PHAT_IS_KNOB(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    knob = PHAT_KNOB(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask =
        gtk_widget_get_events (widget) | 
        GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
        GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
        GDK_POINTER_MOTION_HINT_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->parent->style, widget->window);

    gdk_window_set_user_data(widget->window, widget);
    knob->pixbuf = gdk_pixbuf_new_from_file(INSTALL_DIR"/phat/pixmaps/knob.png",&gerror);
    gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);

    knob->mask_gc = gdk_gc_new(widget->window);
    gdk_gc_copy(knob->mask_gc, widget->style->bg_gc[GTK_STATE_NORMAL]);
    gdk_gc_set_clip_mask(knob->mask_gc, knob->mask);

    knob->red_gc = gdk_gc_new(widget->window);
    gdk_gc_copy(knob->red_gc, widget->style->bg_gc[GTK_STATE_NORMAL]);
    gdk_colormap_alloc_color(attributes.colormap, &color, FALSE, TRUE);
    gdk_gc_set_foreground(knob->red_gc, &color);
}

static void phat_knob_size_request (GtkWidget *widget, GtkRequisition *requisition) {
    requisition->width = KNOB_SIZE;
    requisition->height = KNOB_SIZE;
}

static void phat_knob_size_allocate (GtkWidget *widget, GtkAllocation *allocation) {
    PhatKnob *knob;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(PHAT_IS_KNOB(widget));
    g_return_if_fail(allocation != NULL);

    widget->allocation = *allocation;
    knob = PHAT_KNOB(widget);

    if (GTK_WIDGET_REALIZED(widget)) {
        gdk_window_move_resize(widget->window,
                               allocation->x, allocation->y,
                               allocation->width, allocation->height);
    }
}

static gint phat_knob_expose(GtkWidget *widget, GdkEventExpose *event) {
    PhatKnob *knob;
    gfloat dx, dy, throw;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(PHAT_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    if (event->count > 0)
        return FALSE;

    knob = PHAT_KNOB(widget);

    // FIXME - somewhere in here, we need to read this from the knob size

    // basically we need to work out if the step size is integer
    // if it is, centre the knob about the vertical

    dx = knob->adjustment_prv->value - knob->adjustment_prv->lower;     // value, from 0
    dy = knob->adjustment_prv->upper - knob->adjustment_prv->lower;     // range


    if (knob->adjustment_prv->step_increment != 1.0f) {
        dx=(int)(51*dx/dy)*50;
    } else {
        throw=4;
        dx=(int)(51*dx/throw+(24-throw))*50;
    }



    gdk_draw_pixbuf(widget->window, knob->mask_gc, knob->pixbuf,
                    dx, 0, 0, 0, KNOB_SIZE, KNOB_SIZE,GDK_RGB_DITHER_NONE,0,0);


    return FALSE;
}

static gint phat_knob_button_press(GtkWidget *widget, GdkEventButton *event) {
    PhatKnob *knob;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(PHAT_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    knob = PHAT_KNOB(widget);

    switch (knob->state) {
    case STATE_IDLE:
        switch (event->button) {
        case 1:
        case 2:
            gtk_grab_add(widget);
            knob->state = STATE_PRESSED;
            knob->saved_x = event->x;
            knob->saved_y = event->y;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static gint phat_knob_button_release(GtkWidget *widget, GdkEventButton *event) {
    PhatKnob *knob;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(PHAT_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    knob = PHAT_KNOB(widget);

    switch (knob->state) {
    case STATE_PRESSED:
        gtk_grab_remove(widget);
        knob->state = STATE_IDLE;

        switch (event->button) {
        case 1:
            knob->adjustment_prv->value -= knob->adjustment_prv->page_increment;
            g_signal_emit (G_OBJECT (knob), signals[VALUE_CHANGED_SIGNAL], 0);
            gtk_signal_emit_by_name(GTK_OBJECT(knob->adjustment_prv), "value_changed");
            break;

        case 3:
            knob->adjustment_prv->value += knob->adjustment_prv->page_increment;
            g_signal_emit (G_OBJECT (knob), signals[VALUE_CHANGED_SIGNAL], 0);
            gtk_signal_emit_by_name(GTK_OBJECT(knob->adjustment_prv), "value_changed");
            break;

        default:
            break;
        }
        break;

    case STATE_DRAGGING:
        gtk_grab_remove(widget);
        knob->state = STATE_IDLE;

        if (knob->policy != GTK_UPDATE_CONTINUOUS && knob->old_value != knob->adjustment_prv->value)
        {
            g_signal_emit (G_OBJECT (knob), signals[VALUE_CHANGED_SIGNAL], 0);
            gtk_signal_emit_by_name(GTK_OBJECT(knob->adjustment_prv), "value_changed");
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static gint phat_knob_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    PhatKnob *knob;
    GdkModifierType mods;
    gint x, y;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(PHAT_IS_KNOB(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    knob = PHAT_KNOB(widget);

    x = event->x;
    y = event->y;

    if (event->is_hint || (event->window != widget->window))
        gdk_window_get_pointer(widget->window, &x, &y, &mods);

    switch (knob->state) {
    case STATE_PRESSED:
        knob->state = STATE_DRAGGING;
        /* fall through */

    case STATE_DRAGGING:
        if (mods & GDK_BUTTON1_MASK) {
            phat_knob_update_mouse(knob, x, y, TRUE);
            return TRUE;
        } else if (mods & GDK_BUTTON3_MASK) {
            phat_knob_update_mouse(knob, x, y, FALSE);
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static gint phat_knob_timer(PhatKnob *knob) {
    g_return_val_if_fail(knob != NULL, FALSE);
    g_return_val_if_fail(PHAT_IS_KNOB(knob), FALSE);

    if (knob->policy == GTK_UPDATE_DELAYED)
    {
        g_signal_emit (G_OBJECT (knob), signals[VALUE_CHANGED_SIGNAL], 0);
        gtk_signal_emit_by_name(GTK_OBJECT(knob->adjustment_prv), "value_changed");
    }
    return FALSE;       /* don't keep running this timer */
}

static void phat_knob_update_mouse_update(PhatKnob *knob) {
    if (knob->policy == GTK_UPDATE_CONTINUOUS)
    {
        gtk_signal_emit_by_name(GTK_OBJECT(knob->adjustment_prv), "value_changed");
        g_signal_emit (G_OBJECT (knob), signals[VALUE_CHANGED_SIGNAL], 0);
    }
    else 
    {
        gtk_widget_draw(GTK_WIDGET(knob), NULL);

        if (knob->policy == GTK_UPDATE_DELAYED) {
            if (knob->timer)
                gtk_timeout_remove(knob->timer);

            knob->timer = gtk_timeout_add (SCROLL_DELAY_LENGTH, (GtkFunction) phat_knob_timer,
                                           (gpointer) knob);
        }
    }
}

static void phat_knob_update_mouse(PhatKnob *knob, gint x, gint y,
                                   gboolean absolute)
{
    gfloat old_value, new_value, dv, dh;
    gdouble angle;

    g_return_if_fail(knob != NULL);
    g_return_if_fail(PHAT_IS_KNOB(knob));

    old_value = knob->adjustment_prv->value;

    angle = atan2(-y + (KNOB_SIZE>>1), x - (KNOB_SIZE>>1));

    if (absolute) {

        angle /= M_PI;
        if (angle < -0.5)
            angle += 2;

        new_value = -(2.0/3.0) * (angle - 1.25);   /* map [1.25pi, -0.25pi] onto [0, 1] */
        new_value *= knob->adjustment_prv->upper - knob->adjustment_prv->lower;
        new_value += knob->adjustment_prv->lower;

    } else {

        dv = knob->saved_y - y; /* inverted cartesian graphics coordinate system */
        dh = x - knob->saved_x;
        knob->saved_x = x;
        knob->saved_y = y;

        if (x >= 0 && x <= KNOB_SIZE)
            dh = 0;  /* dead zone */
        else {
            angle = cos(angle);
            dh *= angle * angle;
        }

        new_value = knob->adjustment_prv->value +
            dv * knob->adjustment_prv->step_increment +
            dh * (knob->adjustment_prv->upper -
                  knob->adjustment_prv->lower) / 200.0f;
    }

    new_value = MAX(MIN(new_value, knob->adjustment_prv->upper),
                    knob->adjustment_prv->lower);

    knob->adjustment_prv->value = new_value;

    if (knob->adjustment_prv->value != old_value)
        phat_knob_update_mouse_update(knob);
}

static void phat_knob_update(PhatKnob *knob) {
    gfloat new_value;

    g_return_if_fail(knob != NULL);
    g_return_if_fail(PHAT_IS_KNOB (knob));

    new_value = knob->adjustment_prv->value;
    if (knob->adjustment_prv->step_increment == 1) new_value = (int)(knob->adjustment_prv->value+0.5);

    if (new_value < knob->adjustment_prv->lower)
        new_value = knob->adjustment_prv->lower;

    if (new_value > knob->adjustment_prv->upper)
        new_value = knob->adjustment_prv->upper;

    if (new_value != knob->adjustment_prv->value) {
        knob->adjustment_prv->value = new_value;
        g_signal_emit (G_OBJECT (knob), signals[VALUE_CHANGED_SIGNAL], 0);
        gtk_signal_emit_by_name(GTK_OBJECT(knob->adjustment_prv), "value_changed");
    }

    gtk_widget_draw(GTK_WIDGET(knob), NULL);
}

static void phat_knob_adjustment_changed(GtkAdjustment *adjustment, gpointer data) {
    PhatKnob *knob;

    g_return_if_fail(adjustment != NULL);
    g_return_if_fail(data != NULL);

    knob = PHAT_KNOB(data);

    if ((knob->old_value != adjustment->value) ||
        (knob->old_lower != adjustment->lower) ||
        (knob->old_upper != adjustment->upper)) {
        phat_knob_update (knob);

        knob->old_value = adjustment->value;
        knob->old_lower = adjustment->lower;
        knob->old_upper = adjustment->upper;
    }
}

static void phat_knob_adjustment_value_changed (GtkAdjustment *adjustment, gpointer data) {
    PhatKnob *knob;

    g_return_if_fail(adjustment != NULL);
    g_return_if_fail(data != NULL);

    knob = PHAT_KNOB(data);

    if (knob->old_value != adjustment->value) {
        phat_knob_update (knob);

        knob->old_value = adjustment->value;
    }

    phat_knob_get_value(knob);  /* update value of external adjustment */
}

static void
phat_knob_external_adjustment_changed(
    GtkAdjustment * adjustment,
    gpointer data)
{
    PhatKnob *knob;

    knob = PHAT_KNOB(data);

    phat_knob_set_range(knob, knob->adjustment->lower, knob->adjustment->upper);
}

static void
phat_knob_external_adjustment_value_changed(
    GtkAdjustment * adjustment,
    gpointer data)
{
    PhatKnob *knob;

    knob = PHAT_KNOB(data);

    phat_knob_update_internal_adjustment(knob);
}
