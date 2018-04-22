bool level_00() {
    const char* base_map =
        "........................"
        ".        ..     *      ."
        ".        ....          ."
        ".         ..           ."
        ".         .   O .      ."
        ".         . .   ..   O ."
        ".........   ..       ..."
        "........................";


    using St = State<8, 24, 2, 1, 5>;
    St::Map map(base_map);
    St st;

    St::Snake red { 'R', 5, 4 };
    red.grow(St::Snake::RIGHT);
    red.grow(St::Snake::RIGHT);
    st.add_snake(red, 0);

    st.print(map);

    return search(st, map);
}
