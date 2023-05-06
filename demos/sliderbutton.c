#include <gtk/gtk.h>
#include <phat/phat.h>

enum
{
    SPACING = 5,
};

static void cb_threshold (GtkSpinButton* spin, PhatSliderButton* button)
{
    phat_slider_button_set_threshold (button, gtk_spin_button_get_value_as_int (spin));
}

static void cb_sensitive (GtkToggleButton* check, GtkWidget* button)
{
    gtk_widget_set_sensitive (button, gtk_toggle_button_get_active (check));
}

int main (int argc, char* argv[])
{
    GtkWidget* window;
    GtkWidget* button;
    GtkWidget* spin;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* check;
    GtkWidget* label;
     
    gtk_init (&argc, &argv);

    /* main window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "SliderButton Demo");
    gtk_container_set_border_width (GTK_CONTAINER (window), SPACING);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit), NULL);

    vbox = gtk_vbox_new (FALSE, SPACING);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);
     
    /* slider button */
    button = phat_slider_button_new_with_range (0, -50, 50, 0.25, 2);
    phat_slider_button_set_format (PHAT_SLIDER_BUTTON (button),
                                   -1, "Value: ", " frobs");
    phat_slider_button_set_threshold (PHAT_SLIDER_BUTTON (button), 10);
    gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, FALSE, 0);
    gtk_widget_show (button);

    /* config row */
    hbox = gtk_hbox_new (FALSE, SPACING);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    /* threshold label */
    label = gtk_label_new ("Threshold:");
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
    gtk_widget_show (label);

    /* threshold spin button */
    spin = gtk_spin_button_new_with_range (1, 100, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 10);
    g_signal_connect (G_OBJECT (spin), "value-changed",
                      G_CALLBACK (cb_threshold), (gpointer) button);
    gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, FALSE, 0);
    gtk_widget_show (spin);

    /* sensitive check button */
    check = gtk_check_button_new_with_label ("Sensitive");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
    g_signal_connect (G_OBJECT (check), "toggled",
                      G_CALLBACK (cb_sensitive), (gpointer) button);
    gtk_box_pack_start (GTK_BOX (hbox), check, TRUE, FALSE, 0);
    gtk_widget_show (check);
     
    gtk_widget_show (window);
    gtk_main ( );
    return 0;
}
