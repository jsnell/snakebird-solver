bool level_06() {
    const char* map =
        "............"
        ".          ."       
        ".    ..    ."
        ".    ..    ."
        ".   ..     ."
        ".  ...     ."
        ".   .  .   ."
        ".   .      ."
        ".      .   ."
        ".  ..~     ."
        ".   ...    ."
        ".          ."
        "~~~~~~~~~~~~";
        

    using St = State<13, 12, 1, 1, 5>;
    St st(map);
    st.add_fruit(6, 5, 0);

    st.set_exit(7, 10);

    St::Snake blue { 'B', 10, 2 };
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::RIGHT);
    st.add_snake(blue, 0);

    st.print(map);

    return search(st, map);
}
