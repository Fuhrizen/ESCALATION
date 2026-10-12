// This optional validation path exercises ASan and UBSan, without process-enumeration
// dependent leak checking. Normal builds and CTest never compile this override.
extern "C" int __lsan_is_turned_off() {
  return 1;
}
