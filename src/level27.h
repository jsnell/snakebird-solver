int level_27() {
    const char* base_map =
        "............"
        ".    O     ."
        ".   ..     ."
        ".   ..     ."
        ".   ..     ."
        ".          ."
        ".          ."
        ". #    #   ."
        ". ..  ..   ."
        ". #  0 #   ."
        ".          ."
        ". >G 0 R<* ."
        ". ......^  ."
        ". ......^  ."
        ". ......   ."
        "~~~~~~~~~~~~";

    using St = State<Setup<16, 12, 1, 2, 5, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
