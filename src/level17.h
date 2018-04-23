int level_17() {
    const char* base_map =
        "............."
        ".           ."
        ".           ."
        ".     .~    ."
        ".     .~    ."
        ".      ~G<< ."
        ".       >R^ ."
        ". ..~  ~..^ ."
        ". ..~~ ~..  ."
        ".  .~  ~.   ."
        ".      *    ."
        ".      ~    ."
        ".     .~    ."
        ".     .~    ."
        ".    ~~~    ."
        "~~~~~~~~~~~~~";

    using St = State<16, 13, 0, 2, 5>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
