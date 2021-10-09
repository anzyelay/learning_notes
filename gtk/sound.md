## canberra play
```vala
private void play_shutter_sound () {
    Canberra.Context context;
    Canberra.Proplist props;

    Canberra.Context.create (out context);
    Canberra.Proplist.create (out props);

    props.sets (Canberra.PROP_EVENT_ID, "screen-capture");
    props.sets (Canberra.PROP_EVENT_DESCRIPTION, _("Screenshot taken"));
    props.sets (Canberra.PROP_CANBERRA_CACHE_CONTROL, "permanent");

    context.play_full (0, props, null);
}
```