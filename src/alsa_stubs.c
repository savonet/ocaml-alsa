/*
 * OCaml bindings for alsa
 *
 * Copyright 2005-2006 Savonet team
 *
 * This file is part of ocaml-alsa.
 *
 * ocaml-alsa is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ocaml-alsa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ocaml-alsa; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Samuel Mimram
 */

/* $Id$ */

#include <string.h>
#include <alsa/asoundlib.h>

#include <caml/alloc.h>
#include <caml/callback.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/misc.h>
#include <caml/mlvalues.h>
#include <caml/signals.h>

#include <assert.h>

CAMLprim value ocaml_alsa_version(value unit)
{
  return caml_copy_string(snd_asoundlib_version());
}

typedef struct my_snd_pcm_t
{
  snd_pcm_t *handle;
  int frame_size;
} my_snd_pcm_t;

static struct custom_operations pcm_handle_ops =
{
  "ocaml_alsa_pcm_handle",
  NULL,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default
};

#define Pcm_val(v) ((my_snd_pcm_t*)Data_custom_val(v))
#define Pcm_handle_val(v) Pcm_val(v)->handle
#define Frame_size_val(v) Pcm_val(v)->frame_size

#define int_of_direction(dir) (Int_val(dir)-1)
#define direction_of_int(dir) (Val_int(dir+1))

static int int_of_pcm_stream(value stream)
{
  int ans = 0;

  while (stream != Val_emptylist)
  {
    switch (Int_val(Field(stream, 0)))
    {
      case 0:
        ans |= SND_PCM_STREAM_PLAYBACK;
        break;

      case 1:
        ans |= SND_PCM_STREAM_CAPTURE;
        break;

      default:
        assert(0);
    }
    stream = Field(stream, 1);
  }
  return ans;
}

static int int_of_pcm_mode(value mode)
{
  int ans = 0;

  while (mode != Val_emptylist)
  {
    switch (Int_val(Field(mode, 0)))
    {
      case 0:
        ans |= SND_PCM_ASYNC;
        break;

      case 1:
        ans |= SND_PCM_NONBLOCK;
        break;

      default:
        assert(0);
    }
    mode = Field(mode, 1);
  }

  return ans;
}

/* Here, we use the type snd_pcm_sframes_t
 * which is long usually and makes sure we
 * avoid long to int overflow in the read/write
 * function's return value. */
static void check_for_err(snd_pcm_sframes_t ret)
{
  if (ret >= 0)
    return;
#ifdef DEBUG
  fprintf(stderr, "[DD] ocaml-alsa error: %s\n", strerror(-ret));
#endif

  switch(-ret)
  {
    case EIO:
      caml_raise_constant(*caml_named_value("alsa_exn_io_error"));
      break;

    case EBUSY:
      caml_raise_constant(*caml_named_value("alsa_exn_device_busy"));
      break;

    case EINVAL:
      caml_raise_constant(*caml_named_value("alsa_exn_invalid_argument"));
      break;

    case EPIPE:
      caml_raise_constant(*caml_named_value("alsa_exn_buffer_xrun"));
      break;

    case ESTRPIPE:
      caml_raise_constant(*caml_named_value("alsa_exn_suspended"));
      break;

    case EBADFD:
      caml_raise_constant(*caml_named_value("alsa_exn_bad_state"));
      break;

    case EINTR:
      caml_raise_constant(*caml_named_value("alsa_exn_interrupted"));
      break;

    case ENOTTY:
    case ENODEV:
      caml_raise_constant(*caml_named_value("alsa_exn_device_busy"));
      break;

    default:
      caml_raise_with_arg(*caml_named_value("alsa_exn_unknown_error"), Val_int(-ret));
      break;
  }
}

CAMLprim value ocaml_snd_int_of_error(value name)
{
  CAMLparam1(name);
  char *s = String_val(name);
  if (!strcmp(s,"alsa_exn_io_error"))
    CAMLreturn(Val_int(-EIO));
  if (!strcmp(s,"alsa_exn_device_busy"))
    CAMLreturn(Val_int(-EBUSY));
  if (!strcmp(s,"alsa_exn_invalid_argument"))
    CAMLreturn(Val_int(-EINVAL));
  if (!strcmp(s,"alsa_exn_buffer_xrun"))
    CAMLreturn(Val_int(-EPIPE));
  if (!strcmp(s,"alsa_exn_suspended"))
    CAMLreturn(Val_int(-ESTRPIPE));
  if (!strcmp(s,"alsa_exn_bad_state"))
    CAMLreturn(Val_int(-EBADFD));
  if (!strcmp(s,"alsa_exn_interrupted"))
    CAMLreturn(Val_int(-EINTR));
  if (!strcmp(s,"alsa_exn_device_busy"))
    CAMLreturn(Val_int(-ENODEV));
 
  caml_failwith("unknown value");
}

CAMLprim value ocaml_snd_string_of_error(value n)
{
  CAMLparam1(n);
  CAMLreturn(caml_copy_string(snd_strerror(-Int_val(n))));
}

static void no_sterr_report_cb(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
}

CAMLprim value ocaml_snd_no_stderr_report(value unit)
{
  CAMLparam1(unit);
  snd_lib_error_set_handler(no_sterr_report_cb);
  CAMLreturn(Val_unit);
}

#define Hw_params_val(v) (*((snd_pcm_hw_params_t**)Data_custom_val(v)))

static void finalize_hw_params(value params)
{
  snd_pcm_hw_params_free(Hw_params_val(params));
}

static struct custom_operations hw_params_ops =
{
  "ocaml_alsa_hw_params",
  finalize_hw_params,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default
};

static value create_hw_params()
{
  CAMLlocal1(ans);
  snd_pcm_hw_params_t *params;

  check_for_err(snd_pcm_hw_params_malloc(&params));
  ans = caml_alloc_custom(&hw_params_ops, sizeof(snd_pcm_hw_params_t*), 0, 1);
  Hw_params_val(ans) = params;

  return ans;
}

CAMLprim value ocaml_snd_pcm_open(value name, value stream, value mode)
{
  CAMLparam3(name, stream, mode);
  CAMLlocal1(ans);
  snd_pcm_sframes_t ret;
  my_snd_pcm_t *hnd;

  ans = caml_alloc_custom(&pcm_handle_ops, sizeof(my_snd_pcm_t), 0, 1);
  hnd = Pcm_val(ans);

  ret = snd_pcm_open(&(hnd->handle), String_val(name), int_of_pcm_stream(stream), int_of_pcm_mode(mode));
  if (ret < 0)
    check_for_err(ret);
  hnd->frame_size = -1;

  CAMLreturn(ans);
}

CAMLprim value ocaml_snd_pcm_close(value handle)
{
  CAMLparam1(handle);
  snd_pcm_close(Pcm_handle_val(handle));
  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_prepare(value handle)
{
  CAMLparam1(handle);
  check_for_err(snd_pcm_prepare(Pcm_handle_val(handle)));
  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_resume(value handle)
{
  CAMLparam1(handle);
  check_for_err(snd_pcm_resume(Pcm_handle_val(handle)));
  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_recover(value handle, value err, value log)
{
  CAMLparam1(handle);
  check_for_err(snd_pcm_recover(Pcm_handle_val(handle),Int_val(err),Bool_val(log)));
  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_wait(value handle, value timeout)
{
  CAMLparam2(handle, timeout);
  int ans;

  ans = snd_pcm_wait(Pcm_handle_val(handle), Int_val(timeout));

  CAMLreturn(Val_int(ans));
}

CAMLprim value ocaml_snd_pcm_readi(value handle_, value dbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, dbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  char *buf;
  snd_pcm_sframes_t ret;

  if (ofs + len * Frame_size_val(handle_) > caml_string_length(dbuf))
    caml_invalid_argument("buffer");
  buf = malloc(len * Frame_size_val(handle_));

  caml_enter_blocking_section();
  ret = snd_pcm_readi(handle, buf, len);
  caml_leave_blocking_section();

  memcpy(String_val(dbuf) + ofs, buf, len * Frame_size_val(handle_));
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_writei(value handle_, value sbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, sbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  char *buf;
  snd_pcm_sframes_t ret;

  if (ofs + len * Frame_size_val(handle_) > caml_string_length(sbuf))
    caml_invalid_argument("buffer");
  buf = malloc(len * Frame_size_val(handle_));
  memcpy(buf, String_val(sbuf) + ofs, len * Frame_size_val(handle_));

  caml_enter_blocking_section();
  ret = snd_pcm_writei(handle, buf, len);
  caml_leave_blocking_section();

  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_readn(value handle_, value dbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, dbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  int chans = Wosize_val(dbuf);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  char **buf;
  int c;
  snd_pcm_sframes_t ret;

  /* TODO: check the size of dbuf */
  buf = malloc(chans * sizeof(char*));
  for(c = 0; c < chans; c++)
    buf[c] = malloc(2 * len);

  caml_enter_blocking_section();
  ret = snd_pcm_readn(handle, (void**)buf, len);
  caml_leave_blocking_section();

  for(c = 0; c < chans; c++)
  {
    memcpy(String_val(Field(dbuf, c)) + ofs, buf[c], 2 * len);
    free(buf[c]);
  }
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_writen(value handle_, value sbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, sbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  int chans = Wosize_val(sbuf);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  char **buf;
  int c;
  snd_pcm_sframes_t ret;

  /* TODO: check the size of sbuf */
  buf = malloc(chans * sizeof(char*));
  for(c = 0; c < chans; c++)
  {
    buf[c] = malloc(2 * len);
    memcpy(buf[c], String_val(Field(sbuf, c)) + ofs, 2 * len);
  }

  caml_enter_blocking_section();
  ret = snd_pcm_writen(handle, (void**)buf, len);
  caml_leave_blocking_section();

  for(c = 0; c < chans; c++)
    free(buf[c]);
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_readn_float(value handle_, value dbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, dbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  int chans = Wosize_val(dbuf);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  float **buf;
  int c, i;
  snd_pcm_sframes_t ret;

  /* TODO: check the size of dbuf */
  buf = malloc(chans * sizeof(float*));
  for(c = 0; c < chans; c++)
    buf[c] = malloc(len * sizeof(float));

  caml_enter_blocking_section();
  ret = snd_pcm_readn(handle, (void**)buf, len);
  caml_leave_blocking_section();

  for(c = 0; c < chans; c++)
  {
    for(i = 0; i < len; i++)
      Store_double_field(Field(dbuf, c), i + ofs, buf[c][i]);
    free(buf[c]);
  }
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_writen_float(value handle_, value fbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, fbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  int chans = Wosize_val(fbuf);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  float **buf;
  int c, i;
  snd_pcm_sframes_t ret;

  /* TODO: check the size of fbuf */
  buf = malloc(chans * sizeof(float*));
  for(c = 0; c < chans; c++)
  {
    buf[c] = malloc(len * sizeof(float));
    for(i = 0; i < len; i++)
      buf[c][i] = Double_field(Field(fbuf, c), i + ofs);
  }

  caml_enter_blocking_section();
  ret = snd_pcm_writen(handle, (void**)buf, len);
  caml_leave_blocking_section();

  for(c = 0; c < chans; c++)
    free(buf[c]);
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_readn_float64(value handle_, value dbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, dbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  int chans = Wosize_val(dbuf);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  double **buf;
  int c, i;
  snd_pcm_sframes_t ret;

  /* TODO: check the size of fbuf */
  buf = malloc(chans * sizeof(double*));
  for(c = 0; c < chans; c++)
    buf[c] = malloc(len * sizeof(double));

  caml_enter_blocking_section();
  ret = snd_pcm_readn(handle, (void**)buf, len);
  caml_leave_blocking_section();

  for(c = 0; c < chans; c++)
  {
    for(i = 0; i < len; i++)
      Store_double_field(Field(dbuf, c), i + ofs, buf[c][i]);
    free(buf[c]);
  }
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_writen_float64(value handle_, value fbuf, value ofs_, value len_)
{
  CAMLparam4(handle_, fbuf, ofs_, len_);
  int len = Int_val(len_);
  int ofs = Int_val(ofs_);
  int chans = Wosize_val(fbuf);
  snd_pcm_t *handle = Pcm_handle_val(handle_);
  double **buf;
  int c, i;
  snd_pcm_sframes_t ret;

  /* TODO: check the size of fbuf */
  buf = malloc(chans * sizeof(double*));
  for(c = 0; c < chans; c++)
  {
    buf[c] = malloc(len * sizeof(double));
    for(i = 0; i < len; i++)
      buf[c][i] = Double_field(Field(fbuf, c), i + ofs);
  }

  caml_enter_blocking_section();
  ret = snd_pcm_writen(handle, (void**)buf, len);
  caml_leave_blocking_section();

  for(c = 0; c < chans; c++)
    free(buf[c]);
  free(buf);
  check_for_err(ret);

  CAMLreturn(Val_int(ret));
}

CAMLprim value ocaml_snd_pcm_start(value handle)
{
  CAMLparam1(handle);
  snd_pcm_sframes_t ret;

  ret = snd_pcm_start(Pcm_handle_val(handle));
  check_for_err(ret);

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_drain(value handle)
{
  CAMLparam1(handle);

  check_for_err(snd_pcm_drain(Pcm_handle_val(handle)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_drop(value handle)
{
  CAMLparam1(handle);

  check_for_err(snd_pcm_drop(Pcm_handle_val(handle)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_pause(value handle, value pause)
{
  CAMLparam2(handle, pause);

  check_for_err(snd_pcm_pause(Pcm_handle_val(handle), Int_val(pause)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_reset(value handle)
{
  CAMLparam1(handle);

  check_for_err(snd_pcm_reset(Pcm_handle_val(handle)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_get_delay(value handle)
{
  CAMLparam1(handle);
  long ans;

  check_for_err(snd_pcm_delay(Pcm_handle_val(handle), &ans));

  CAMLreturn(Val_int(ans));
}

CAMLprim value ocaml_snd_pcm_get_state(value handle)
{
  CAMLparam1(handle);
  snd_pcm_state_t state;
  int ans;

  state = snd_pcm_state(Pcm_handle_val(handle));
  switch (state)
  {
    case SND_PCM_STATE_OPEN:
      ans = 0;
      break;

    case SND_PCM_STATE_SETUP:
      ans = 1;
      break;

    case SND_PCM_STATE_PREPARED:
      ans = 2;
      break;

    case SND_PCM_STATE_RUNNING:
      ans = 3;
      break;

    case SND_PCM_STATE_XRUN:
      ans = 4;
      break;

    case SND_PCM_STATE_DRAINING:
      ans = 5;
      break;

    case SND_PCM_STATE_PAUSED:
      ans = 6;
      break;

    case SND_PCM_STATE_SUSPENDED:
      ans = 7;
      break;

    case SND_PCM_STATE_DISCONNECTED:
      ans = 8;
      break;

    default:
      assert(0);
  }

  CAMLreturn(Val_int(ans));
}

CAMLprim value ocaml_snd_pcm_get_params(value handle)
{
  CAMLparam1(handle);
  CAMLlocal1(ans);

  ans = create_hw_params();
  check_for_err(snd_pcm_hw_params_any(Pcm_handle_val(handle), Hw_params_val(ans)));

  CAMLreturn(ans);
}

CAMLprim value ocaml_snd_pcm_set_params(value handle, value params)
{
  CAMLparam2(handle, params);

  check_for_err(snd_pcm_hw_params(Pcm_handle_val(handle), Hw_params_val(params)));
  /* TODO: compute real value */
  Frame_size_val(handle) = 4;

  CAMLreturn(Val_unit);
}

static int int_of_access(value access)
{
  int a = Int_val(access);

  if (!a--)
    return SND_PCM_ACCESS_RW_INTERLEAVED;
  else if (!a--)
    return SND_PCM_ACCESS_RW_NONINTERLEAVED;
  else
    assert(0);
}

CAMLprim value ocaml_snd_pcm_set_access(value handle, value params, value access)
{
  CAMLparam3(handle, params, access);

  check_for_err(snd_pcm_hw_params_set_access(Pcm_handle_val(handle), Hw_params_val(params), int_of_access(access)));

  CAMLreturn(Val_unit);
}

static int int_of_format(value format)
{
  int f = Int_val(format);

  if (!f--)
    return SND_PCM_FORMAT_S16_LE;
  else if (!f--)
    return SND_PCM_FORMAT_S24_3LE;
  else if (!f--)
    return SND_PCM_FORMAT_FLOAT;
  else if (!f--)
    return SND_PCM_FORMAT_FLOAT64;
  else
    assert(0);
}

CAMLprim value ocaml_snd_pcm_set_format(value handle, value params, value format)
{
  CAMLparam3(handle, params, format);

  check_for_err(snd_pcm_hw_params_set_format(Pcm_handle_val(handle), Hw_params_val(params), int_of_format(format)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_set_rate_near(value handle, value params, value rate_, value dir_)
{
  CAMLparam4(handle, params, rate_, dir_);
  unsigned int rate = Int_val(rate_);
  int dir = int_of_direction(dir_);

  check_for_err(snd_pcm_hw_params_set_rate_near(Pcm_handle_val(handle), Hw_params_val(params), &rate, &dir));

  CAMLreturn(Val_int(rate));
}

CAMLprim value ocaml_snd_pcm_set_channels(value handle, value params, value chans)
{
  CAMLparam3(handle, params, chans);

  check_for_err(snd_pcm_hw_params_set_channels(Pcm_handle_val(handle), Hw_params_val(params), Int_val(chans)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_set_periods(value handle, value params, value periods, value dir)
{
  CAMLparam4(handle, params, periods, dir);

  check_for_err(snd_pcm_hw_params_set_periods(Pcm_handle_val(handle), Hw_params_val(params), Int_val(periods), int_of_direction(dir)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_get_periods_min(value params)
{
  CAMLparam1(params);
  CAMLlocal1(result);
  int ret, dir ;
  unsigned int periods ;

  ret = snd_pcm_hw_params_get_periods_min(Hw_params_val(params), &periods, &dir);
  check_for_err(ret);
  result = caml_alloc_tuple(2);
  Store_field(result,0,Val_int(periods));
  Store_field(result,1,direction_of_int(dir));
  CAMLreturn(result);
}

CAMLprim value ocaml_snd_pcm_get_periods_max(value params)
{
  CAMLparam1(params);
  CAMLlocal1(result);
  int ret, dir ;
  unsigned int periods ;

  ret = snd_pcm_hw_params_get_periods_max(Hw_params_val(params), &periods, &dir);
  check_for_err(ret);
  result = caml_alloc_tuple(2);
  Store_field(result,0,Val_int(periods));
  Store_field(result,1,direction_of_int(dir));
  CAMLreturn(result);
}

CAMLprim value ocaml_snd_pcm_set_buffer_size(value handle, value params, value size)
{
  CAMLparam3(handle, params, size);

  check_for_err(snd_pcm_hw_params_set_buffer_size(Pcm_handle_val(handle), Hw_params_val(params), Int_val(size)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_set_buffer_size_near(value handle, value params, value size)
{
  CAMLparam3(handle, params, size);
  snd_pcm_uframes_t s = Int_val(size);

  check_for_err(snd_pcm_hw_params_set_buffer_size_near(Pcm_handle_val(handle), Hw_params_val(params), &s));

  CAMLreturn(Val_int(s));
}

CAMLprim value ocaml_snd_pcm_get_buffer_size_min(value params)
{
  CAMLparam1(params);
  snd_pcm_uframes_t ans;

  check_for_err(snd_pcm_hw_params_get_buffer_size_min(Hw_params_val(params), &ans));

  CAMLreturn(Val_int(ans));
}

CAMLprim value ocaml_snd_pcm_get_buffer_size_max(value params)
{
  CAMLparam1(params);
  snd_pcm_uframes_t ans;

  check_for_err(snd_pcm_hw_params_get_buffer_size_max(Hw_params_val(params), &ans));

  CAMLreturn(Val_int(ans));
}

static int get_access(value access)
{
  int a = Int_val(access);

  if (!a--)
    return SND_PCM_ACCESS_RW_INTERLEAVED;
  else if (!a--)
    return SND_PCM_ACCESS_RW_NONINTERLEAVED;
  else
    assert(0);
}


CAMLprim value ocaml_snd_set_access(value handle, value params, value access)
{
  CAMLparam3(handle, params, access);

  check_for_err(snd_pcm_hw_params_set_access(Pcm_handle_val(handle), Hw_params_val(params), get_access(access)));

  CAMLreturn(Val_unit);
}

CAMLprim value ocaml_snd_pcm_set_nonblock(value handle, value nonblocking)
{
  CAMLparam1(handle);

  check_for_err(snd_pcm_nonblock(Pcm_handle_val(handle), Bool_val(nonblocking)));

  CAMLreturn(Val_unit);
}
