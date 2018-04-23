int level_07() {
    const char* base_map =
        ".............."        
        ".       *    ."
        ".            ."
        ".    .       ."
        ".            ."
        ". .          ."
        ". .          ."
        ". .  .   B<< ."
        ".    .  G<<^ ."
        ".       .... ."
        "~~~~~~~~~~~~~~";

    using St = State<11, 14, 0, 2, 4>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
