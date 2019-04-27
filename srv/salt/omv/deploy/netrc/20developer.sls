{% set config = salt['omv_conf.get']('conf.service.developer') %}

create_netrc_developer:
  file.accumulated:
    - filename: "/root/.netrc"
    - text:
      - machine github.com
      - login {{ config.ghusername }}
      - password {{ config.ghpassword }}
      - protcol https
      - ""
    - require_in:
      - file: append_netrc_entries
