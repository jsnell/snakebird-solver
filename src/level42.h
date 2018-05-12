int level_42() {
    const char* base_map =
        "..............."
        ".        ##.  ."
        ".        ...  ."
        ".         .   ."
        ".    . >R . * ."
        ".  .   G<     ."
        ".    O#O.  .  ."
        ".   #      .  ."
        ".   .. .   .  ."
        ".   .. .   .  ."
        ".   .. .   .  ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<12, 15, 2, 2, 4, 0>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
