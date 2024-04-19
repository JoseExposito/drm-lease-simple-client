project('drm-lease-simple-client', 'c',
        version: '1.0.0',
        license: 'GPL-3.0-only',
        default_options: [
            'c_std=c11',
            'warning_level=2',
        ],
        meson_version: '>= 0.62.0'
)

dep_wl_client = dependency('wayland-client')

wl_mod = import('unstable-wayland')
wl_drm_lease_xml = wl_mod.find_protocol('drm-lease', state: 'staging', version: 1)
wl_drm_lease = wl_mod.scan_xml(wl_drm_lease_xml)

executable('drm-lease-simple-client',
           'main.c', wl_drm_lease,
           dependencies: [dep_wl_client],
)
