@0xaa73873c33abc4a9;

interface Input {
  keypress @0 (key :UInt32);
}

interface Plane {
  newPlane @0 () -> (plane :Plane);
  putText @1 (text :Text, y :UInt32 = 0) -> (bytes: UInt64);
  putStr @2 (text :Text, y :Int32 = -1, x :Int32 = -1) -> (count: Int32);
}

interface Notcurses {
  root @0 (input :Input) -> (plane :Plane);
}
