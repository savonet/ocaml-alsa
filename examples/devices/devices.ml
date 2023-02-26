open Alsa

let samplerate = 44100
let channels = 2

let () =
  Printf.printf "Using ALSA %s.\n%!" (Alsa.get_version ());
  device_name_hints ()
