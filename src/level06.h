int level_06() {
    const char* base_map =
        "............"
        ".          ."       
        ".    ..    ."
        ".    ..    ."
        ".   ..     ."
        ".  ...     ."
        ".   .O .   ."
        ".   .     *."
        ". >B   .   ."
        ". ^..~     ."
        ". ^ ...    ."
        ".          ."
        "~~~~~~~~~~~~";
        

    using St = State<13, 12, 1, 1, 5>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    return search(st, map);
}
