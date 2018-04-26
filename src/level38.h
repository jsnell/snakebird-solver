int level_38() {
    const char* base_map =
        "..............."
        ".             ."
        ".             ."
        ".         *   ."
        ".  T          ."
        ".       G<<<  ."
        ".       ..... ."
        ".       T .   ."
        ".       . .   ."
        ".  >>>R ...   ."
        ". .......     ."
        "~~~~~~~~~~~~~~~";

    using St = State<12, 15, 0, 2, 4, 0, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
