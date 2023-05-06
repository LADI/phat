#include <gtk/gtk.h>
#include <phat/phat.h>

enum
{
    SPACING = 5,
};

void motion_cb(PhatPad* pad)
{
    printf("for signals xtilt %f x %f y %f\n", phat_pad_get_xtilt(pad)->value, phat_pad_get_x(pad)->value, phat_pad_get_y(pad)->value);
}  


void input_dialog_destroy (GtkWidget *w, gpointer data)
{
    *((GtkWidget **)data) = NULL;
}

void create_input_dialog ()
{
    static GtkWidget *inputd = NULL;

    if (!inputd)
    {
        inputd = gtk_input_dialog_new();

        g_signal_connect (G_OBJECT (inputd), "destroy",
                          G_CALLBACK (input_dialog_destroy), (gpointer) &inputd);
        g_signal_connect_swapped (G_OBJECT (GTK_INPUT_DIALOG (inputd)->close_button),
                                  "clicked",
                                  G_CALLBACK (gtk_widget_hide),
                                  G_OBJECT (inputd));
        gtk_widget_hide (GTK_INPUT_DIALOG (inputd)->save_button);

        gtk_widget_show (inputd);
    }
    else
    {
        if (!GTK_WIDGET_MAPPED (inputd))
            gtk_widget_show (inputd);
        else
            gdk_window_raise (inputd->window);
    }
}

int main (int argc, char* argv[])
{
    GtkWidget* window;
    GtkWidget* pad;
    GtkWidget* button;
    GtkWidget* vbox;
          
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
     
    /*  pad */
    pad = phat_pad_new ();
    PHAT_PAD(pad)->x_is_log = TRUE;
    gtk_box_pack_start (GTK_BOX (vbox), pad, TRUE, FALSE, 0);
    gtk_widget_show (pad);
    g_signal_connect (G_OBJECT (pad), "value-changed",
                      G_CALLBACK (motion_cb), NULL);

    button = gtk_button_new_with_label ("Input Dialog");
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (create_input_dialog), NULL);
    gtk_widget_show (button);

    gtk_widget_show (window);
    gtk_main ( );
    return 0;
}
