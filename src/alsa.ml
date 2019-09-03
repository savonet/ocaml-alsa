(*
   Copyright 2005-2007 Savonet team

   This file is part of Ocaml-alsa.

   Ocaml-alsa is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   Ocaml-alsa is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Ocaml-alsa; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*)

external get_version : unit -> string = "ocaml_alsa_version"

exception Buffer_xrun

exception Bad_state

exception Suspended

exception IO_error

exception Device_busy

exception Invalid_argument

exception Device_removed

exception Interrupted

exception Unknown_error of int

type direction = Dir_down | Dir_eq | Dir_up

let _ =
  Callback.register_exception "alsa_exn_buffer_xrun" Buffer_xrun;
  Callback.register_exception "alsa_exn_bad_state" Bad_state;
  Callback.register_exception "alsa_exn_suspended" Suspended;
  Callback.register_exception "alsa_exn_io_error" IO_error;
  Callback.register_exception "alsa_exn_device_busy" Device_busy;
  Callback.register_exception "alsa_exn_invalid_argument" Invalid_argument;
  Callback.register_exception "alsa_exn_device_removed" Device_removed;
  Callback.register_exception "alsa_exn_interrupted" Interrupted;
  Callback.register_exception "alsa_exn_unknown_error" (Unknown_error 0)

external no_stderr_report : unit -> unit = "ocaml_snd_no_stderr_report"

external string_of_error : int -> string = "ocaml_snd_string_of_error"

external int_of_error : string -> int = "ocaml_snd_int_of_error"

let int_of_error e = 
  let f = int_of_error in
  match e with
    | Buffer_xrun      -> f "alsa_exn_buffer_xrun"
    | Bad_state        -> f "alsa_exn_bad_state"
    | Suspended        -> f "alsa_exn_suspended"
    | IO_error         -> f "alsa_exn_io_error"
    | Device_busy      -> f "alsa_exn_device_busy"
    | Invalid_argument -> f "alsa_exn_invalid_argument"
    | Device_removed   -> f "alsa_exn_device_removed"
    | Interrupted      -> f "alsa_exn_interrupted"
    | Unknown_error x  -> x
    | _                -> raise e

let string_of_error e = string_of_error (int_of_error e)

module Pcm =
struct
  type handle

  type params

  type stream = Playback | Capture

  type mode = Async | Non_blocking

  type state = St_open | St_setup | St_prepared | St_running | St_xrun | St_draining | St_paused | St_suspended | St_disconnected

  external open_pcm : string -> stream list -> mode list -> handle = "ocaml_snd_pcm_open"

  external close : handle -> unit = "ocaml_snd_pcm_close"

  external prepare : handle -> unit = "ocaml_snd_pcm_prepare"

  external resume : handle -> unit = "ocaml_snd_pcm_resume"

  external recover : handle -> int -> bool -> unit = "ocaml_snd_pcm_recover"

  let recover ?(verbose=false) h e = 
    recover h (int_of_error e) verbose 

  external start : handle -> unit = "ocaml_snd_pcm_start"

  external drain : handle -> unit = "ocaml_snd_pcm_drain"

  external drop : handle -> unit = "ocaml_snd_pcm_drop"

  external pause : handle -> bool -> unit = "ocaml_snd_pcm_pause"

  external reset : handle -> unit = "ocaml_snd_pcm_reset"

  external wait : handle -> int -> bool = "ocaml_snd_pcm_wait"

  external readi : handle -> string -> int -> int -> int = "ocaml_snd_pcm_readi"

  external writei : handle -> string -> int -> int -> int = "ocaml_snd_pcm_writei"

  external readn : handle -> string array -> int -> int -> int = "ocaml_snd_pcm_readn"

  external writen : handle -> string array -> int -> int -> int = "ocaml_snd_pcm_writen"

  external readn_float : handle -> float array array -> int -> int -> int = "ocaml_snd_pcm_readn_float"

  external writen_float : handle -> float array array -> int -> int -> int = "ocaml_snd_pcm_writen_float"

  external readn_float_ba : handle -> (float, Bigarray.float32_elt, Bigarray.c_layout) Bigarray.Array1.t array -> int = "ocaml_snd_pcm_readn_float_ba"

  external writen_float_ba : handle -> (float, Bigarray.float32_elt, Bigarray.c_layout) Bigarray.Array1.t array -> int = "ocaml_snd_pcm_writen_float_ba"

  external readn_float64 : handle -> float array array -> int -> int -> int = "ocaml_snd_pcm_readn_float64"

  external writen_float64 : handle -> float array array -> int -> int -> int = "ocaml_snd_pcm_writen_float64"

  external get_state : handle -> state = "ocaml_snd_pcm_get_state"

  external get_delay : handle -> int = "ocaml_snd_pcm_get_delay"

  external get_params : handle -> params = "ocaml_snd_pcm_get_params"

  external set_params : handle -> params -> unit = "ocaml_snd_pcm_set_params"

  type access =
    | Access_rw_interleaved
    | Access_rw_noninterleaved

  external set_access : handle -> params -> access -> unit = "ocaml_snd_set_access"

  type fmt =
    | Format_s16_le (* TODO *)
    | Format_s24_3le
    | Format_float
    | Format_float64

  external set_format : handle -> params -> fmt -> unit = "ocaml_snd_pcm_set_format"

  external set_rate_near : handle -> params -> int -> direction -> int = "ocaml_snd_pcm_set_rate_near"

  external set_channels : handle -> params -> int -> unit = "ocaml_snd_pcm_set_channels"

  external set_periods : handle -> params -> int -> direction -> unit = "ocaml_snd_pcm_set_periods"

  external get_periods_max : params -> int*direction = "ocaml_snd_pcm_get_periods_max"

  external get_periods_min : params -> int*direction = "ocaml_snd_pcm_get_periods_min"

  external set_buffer_size : handle -> params -> int -> unit = "ocaml_snd_pcm_set_buffer_size"

  external set_buffer_size_near : handle -> params -> int -> int = "ocaml_snd_pcm_set_buffer_size_near"

  external get_buffer_size_min : params -> int = "ocaml_snd_pcm_get_buffer_size_min"

  external get_buffer_size_max : params -> int = "ocaml_snd_pcm_get_buffer_size_max"

  external set_nonblock : handle -> bool -> unit = "ocaml_snd_pcm_set_nonblock"

  (* TODO *)
  let get_frame_size params = 4
end
