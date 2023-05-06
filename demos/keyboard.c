#include <gtk/gtk.h>
#include <phat/phat.h>

const int NUMKEYS = 128;

GtkWidget* mega_label;

static void pressed(GtkWidget* widget, int key, gpointer data)
{
    char* s = g_strdup_printf("Key Pressed: %d", key);

    gtk_label_set_text(GTK_LABEL(mega_label), s);

    g_free(s);
}


static void released(GtkWidget* widget, int key, gpointer data)
{
    char* s = g_strdup_printf("Key Released: %d", key);

    gtk_label_set_text(GTK_LABEL(mega_label), s);

    g_free(s);
}


int main (int argc, char* argv[])
{
    GtkWidget* window;
    GtkWidget* keyboard;
    GtkWidget* main_hbox;
    GtkWidget* hbox;
    GtkWidget* vbox;
    GtkWidget* scroll;
    GtkAdjustment* adj;
     
    gtk_init (&argc, &argv);

    /* main window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Keyboard Demo");
    gtk_container_set_border_width (GTK_CONTAINER (window), 12);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit), NULL);

    /* main hbox */
    main_hbox = gtk_hbox_new (FALSE, 12);
    gtk_container_add (GTK_CONTAINER (window), main_hbox);
    gtk_widget_show (main_hbox);

    /* hbox */
    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start(GTK_BOX(main_hbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    /* vkeyboard */
    adj = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    keyboard = phat_vkeyboard_new(adj, NUMKEYS, TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), keyboard, FALSE, FALSE, 0);
    gtk_widget_show(keyboard);

    g_signal_connect(G_OBJECT(keyboard), "key-pressed",
                     G_CALLBACK(pressed), NULL);
    g_signal_connect(G_OBJECT(keyboard), "key-released",
                     G_CALLBACK(released), NULL);

    /* scrollbar */
    scroll = gtk_vscrollbar_new(adj);
    gtk_box_pack_start(GTK_BOX(hbox), scroll, FALSE, FALSE, 0);
    gtk_widget_show(scroll);

    /* vbox */
    vbox = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start(GTK_BOX(main_hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);

    /* hkeyboard */
    adj = (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    keyboard = phat_hkeyboard_new(adj, NUMKEYS, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), keyboard, FALSE, FALSE, 0);
    gtk_widget_show(keyboard);

    g_signal_connect(G_OBJECT(keyboard), "key-pressed",
                     G_CALLBACK(pressed), NULL);
    g_signal_connect(G_OBJECT(keyboard), "key-released",
                     G_CALLBACK(released), NULL);

    /* scrollbar */
    scroll = gtk_hscrollbar_new(adj);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, FALSE, FALSE, 0);
    gtk_widget_show(scroll);

    /* label */
    mega_label = gtk_label_new("Press a Key");
    gtk_box_pack_start(GTK_BOX(vbox), mega_label, TRUE, TRUE, 18);
    gtk_widget_show(mega_label);
         
    gtk_widget_show (window);
    gtk_main ( );
    return 0;
}
