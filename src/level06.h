bool level_06() {
    const char* base_map =
        "............"
        ".          ."       
        ".    ..    ."
        ".    ..    ."
        ".   ..     ."
        ".  ...     ."
        ".   .O .   ."
        ".   .     *."
        ".      .   ."
        ".  ..~     ."
        ".   ...    ."
        ".          ."
        "~~~~~~~~~~~~";
        

    using St = State<13, 12, 1, 1, 5>;
    St::Map map(base_map);
    St st;

    St::Snake blue { 'B', 10, 2 };
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::RIGHT);
    st.add_snake(blue, 0);

    st.print(map);

    return search(st, map);
}
