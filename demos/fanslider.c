#include <stdio.h>
#include <gtk/gtk.h>
#include <phat/phat.h>

enum
{
    SPACING = 5,
};

static void cb_inverted (GtkCheckButton* check, PhatFanSlider* slider)
{
    phat_fan_slider_set_inverted (slider, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
}

void cb_slider_value (PhatFanSlider* slider)
{
    printf("slider val %f \n", phat_fan_slider_get_value(slider));
}

static void cb_sensitive (GtkCheckButton* check, GtkWidget* slider)
{
    gtk_widget_set_sensitive (slider, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
}

static void cb_lower (GtkAdjustment* lower, GtkAdjustment* slider)
{
    if (lower->value >= slider->upper)
    {
        gtk_adjustment_set_value (lower, slider->upper - 0.01);
        return;
    }

    if (lower->value > slider->value)
    {
        gtk_adjustment_set_value (lower, slider->value);
        return;
    }

    slider->lower = lower->value;
    gtk_adjustment_changed (slider);
}

static void cb_upper (GtkAdjustment* upper, GtkAdjustment* slider)
{
    if (upper->value <= slider->lower)
    {
        gtk_adjustment_set_value (upper, slider->lower + 0.01);
        return;
    }

    if (upper->value < slider->value)
    {
        gtk_adjustment_set_value (upper, slider->value);
        return;
    }

    slider->upper = upper->value;
    gtk_adjustment_changed (slider);
}

int main (int argc, char* argv[])
{
    GtkWidget* window;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* slider;
    GtkWidget* label;
    GtkWidget* spin;
    GtkWidget* check;
    GtkAdjustment* adj;
    GtkAdjustment* spin_adj;
          
    gtk_init (&argc, &argv);

    /* main window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Fanslider Demo");
    gtk_container_set_border_width (GTK_CONTAINER (window), SPACING);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit), NULL);

    vbox = gtk_vbox_new (FALSE, SPACING);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    /* horizontal slider */
    adj = (GtkAdjustment*) gtk_adjustment_new (0, 1, 2, .01, .1, 0);
    slider = phat_hfan_slider_new (adj);
    g_signal_connect (G_OBJECT (slider), "value-changed",
                      G_CALLBACK (cb_slider_value), (gpointer) slider);
    gtk_box_pack_start (GTK_BOX (vbox), slider, TRUE, TRUE, 0);
    gtk_widget_show (slider);

    /* horizontal slider controls */
    hbox = gtk_hbox_new (FALSE, SPACING);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new ("Value:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    spin = gtk_spin_button_new (adj, 0, 2);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
    gtk_widget_show (spin);

    label = gtk_label_new ("Lower:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    spin_adj = (GtkAdjustment*) gtk_adjustment_new (1, 1, 300, .01, 0, 0);
    g_signal_connect (G_OBJECT (spin_adj), "value-changed",
                      G_CALLBACK (cb_lower), (gpointer) adj);
    spin = gtk_spin_button_new (spin_adj, 0, 2);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
    gtk_widget_show (spin);
     
    label = gtk_label_new ("Upper:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    spin_adj = (GtkAdjustment*) gtk_adjustment_new (20, 1, 20000, .01, 0, 0);
    g_signal_connect (G_OBJECT (spin_adj), "value-changed",
                      G_CALLBACK (cb_upper), (gpointer) adj);
    spin = gtk_spin_button_new (spin_adj, 0, 2);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
    gtk_widget_show (spin);

    check = gtk_check_button_new_with_label ("Inverted");
    g_signal_connect (G_OBJECT (check), "toggled",
                      G_CALLBACK (cb_inverted), (gpointer) slider);
    gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
    gtk_widget_show (check);
     
    check = gtk_check_button_new_with_label ("Sensitive");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
    g_signal_connect (G_OBJECT (check), "toggled",
                      G_CALLBACK (cb_sensitive), (gpointer) slider);
    gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
    gtk_widget_show (check);

    /* vertical slider */
    adj = (GtkAdjustment*) gtk_adjustment_new (0, 0, 1, .01, .1, 0);
    slider = phat_vfan_slider_new (adj);
    gtk_box_pack_start (GTK_BOX (vbox), slider, TRUE, TRUE, 0);
    gtk_widget_show (slider);

    /* vertical slider controls */
    hbox = gtk_hbox_new (FALSE, SPACING);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new ("Value:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    spin = gtk_spin_button_new (adj, 0, 2);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
    gtk_widget_show (spin);

    label = gtk_label_new ("Lower:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    spin_adj = (GtkAdjustment*) gtk_adjustment_new (0, -5, 0, .01, 0, 0);
    g_signal_connect (G_OBJECT (spin_adj), "value-changed",
                      G_CALLBACK (cb_lower), (gpointer) adj);
    spin = gtk_spin_button_new (spin_adj, 0, 2);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
    gtk_widget_show (spin);
     
    label = gtk_label_new ("Upper:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    spin_adj = (GtkAdjustment*) gtk_adjustment_new (1, 0, 5, .01, 0, 0);
    g_signal_connect (G_OBJECT (spin_adj), "value-changed",
                      G_CALLBACK (cb_upper), (gpointer) adj);
    spin = gtk_spin_button_new (spin_adj, 0, 2);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
    gtk_widget_show (spin);

    check = gtk_check_button_new_with_label ("Inverted");
    g_signal_connect (G_OBJECT (check), "toggled",
                      G_CALLBACK (cb_inverted), (gpointer) slider);
    gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
    gtk_widget_show (check);

    check = gtk_check_button_new_with_label ("Sensitive");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
    g_signal_connect (G_OBJECT (check), "toggled",
                      G_CALLBACK (cb_sensitive), (gpointer) slider);
    gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, TRUE, 0);
    gtk_widget_show (check);

    gtk_widget_show (window);
    gtk_main ( );
    return 0;
}

