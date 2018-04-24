int level_33() {
    const char* base_map =
        "..............."
        ".             ."
        ".    *        ."
        ".             ."
        ".    ###..... ."
        ".    O   T. . ."
        ".    .##  O . ."
        ".        .  . ."
        ".    .      . ."
        ". #   T     . ."
        ".        G< . ."
        ".   ##.#..^ . ."
        ".   ... ..  . ."
        "~~~~~~~~~~~~~~~";

    using St = State<14, 15, 2, 1, 0, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}