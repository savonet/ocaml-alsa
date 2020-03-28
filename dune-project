(lang dune 2.0)
(version 0.2.5)
(name alsa)
(source (github savonet/ocaml-alsa))
(license GPL-2.0)
(authors "The Savonet Team <savonet-users@lists.sourceforge.net>")
(maintainers "The Savonet Team <savonet-users@lists.sourceforge.net>")

(generate_opam_files true)

(package
 (name alsa)
 (synopsis "Bindings for the ALSA library which provides functions for using soundcards")
 (depends
  (dune (> 2.0))
  dune-configurator)
)
