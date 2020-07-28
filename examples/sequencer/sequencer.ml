open Alsa

let () =
  Printf.printf "Using ALSA %s\n\n%!" (Alsa.get_version ());
  let seq = Sequencer.create "default" `Input in
  Sequencer.set_client_name seq "OCaml test client";
  let port = Sequencer.create_port seq "Test port" [Port_cap_write; Port_cap_subs_write] [Port_type_MIDI_generic] in
  Printf.printf "port: %d\n%!" port;
  Sequencer.subscribe_read_all seq port;
  while true do
    match (Sequencer.input_event seq).ev_event with
    | Sequencer.Event.Note_on n -> Printf.printf "note on: %d\n%!" n.note_note
    | Sequencer.Event.Note_off n -> Printf.printf "note off: %d\n%!" n.note_note
    | Sequencer.Event.Controller c -> Printf.printf "controller: %d\n%!" c.controller_value
    | Sequencer.Event.Pitch_bend c -> Printf.printf "pitch bend: %d\n%!" c.controller_value
    | _ -> Printf.printf "ignored event\n%!";
  done
              
