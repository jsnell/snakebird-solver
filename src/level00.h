bool level_00() {
    const char* map =
        "........................"
        ".        ..            ."
        ".        ....          ."
        ".         ..           ."
        ".         .     .      ."
        ".         . .   ..     ."
        ".........   ..       ..."
        "........................";


    using St = State<8, 24, 2, 1, 5>;
    St st(map);
    st.add_fruit(4, 14, 0);
    st.add_fruit(5, 21, 1);

    st.set_exit(1, 16);

    St::Snake red { 'R', 5, 4 };
    red.grow(St::Snake::RIGHT);
    red.grow(St::Snake::RIGHT);
    st.add_snake(red, 0);

    st.print(map);

    return search(st, map);
}
