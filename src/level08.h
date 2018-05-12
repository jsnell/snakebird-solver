int level_08() {
    const char* base_map =
        ".................."        
        ".              * ."
        ".                ."
        ".  >>R        ...."
        ". >^G<<     ~~~..."
        ".......~~~  ~    ."
        ". .....  ~~.~    ."
        "~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<8, 18, 0, 2, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
