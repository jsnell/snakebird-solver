bool level_19() {
    const char* base_map =
        "................"        
        ".              ."
        ".   ~          ."
        ".~  .          ."
        ".. *          O."
        ".   B<         ."
        ".  >>G         ."
        ".  ^R<         ."
        ".  ^.^         ."
        ".  ^.^         ."
        ".   .^         ."
        ".   .          ."
        "~~~~~~~~~~~~~~~~";

    using St = State<13, 16, 1, 3, 7>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    return search(st, map);
}
