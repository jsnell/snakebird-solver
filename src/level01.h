bool level_01() {
    const char* base_map =
        "........"        
        ".   *  ."
        ".      ."
        "..     ."
        ".O  .O.."
        ".      ."
        ". .    ."
        ". .... ."
        ". .... ."
        ". ...  ."
        "~~~~~~~~";

    using St = State<11, 8, 2, 1, 4>;
    St::Map map(base_map);
    St st;

    St::Snake green { 'G', 6, 3 };
    green.grow(St::Snake::RIGHT);
    st.add_snake(green, 0);

    st.print(map);

    return search(st, map);
}
