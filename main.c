#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <threads.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xf86drm.h>
#include "drm-lease-v1-client-protocol.h"

#define TARGET_CONNECTOR_DESC "DEL DELL U2422H"

struct state {
    struct wl_compositor *compositor;

    // For simplicity, handle only one device
    struct wp_drm_lease_device_v1 *drm_lease_device;
    int32_t drm_lease_device_fd;

    // For simplicity, handle only the connector with description TARGET_CONNECTOR_DESC
    struct wp_drm_lease_connector_v1 *drm_lease_connector;
};

// --------------------------- drm_lease: connector ----------------------------

static void wp_drm_lease_connector_v1_listener_handle_name(void *data,
		     struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
		     const char *name)
{
    printf("wp_drm_lease_connector_v1_listener_handle_name: %s\n", name);

    struct state *state = data;
}

static void wp_drm_lease_connector_v1_listener_handle_description(void *data,
			    struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
			    const char *description)
{
    printf("wp_drm_lease_connector_v1_listener_handle_description: %s\n", description);

    struct state *state = data;

    if (strcmp(description, TARGET_CONNECTOR_DESC) == 0) {
        printf("\tTarget connector found!\n");
        state->drm_lease_connector = wp_drm_lease_connector_v1;
    }
}

static void wp_drm_lease_connector_v1_listener_handle_connector_id(void *data,
			     struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
			     uint32_t connector_id)
{
    printf("wp_drm_lease_connector_v1_listener_handle_connector_id: %d\n", connector_id);

    struct state *state = data;
}

static void wp_drm_lease_connector_v1_listener_handle_done(void *data,
		     struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
    printf("wp_drm_lease_connector_v1_listener_handle_done\n");

    struct state *state = data;
}

static void wp_drm_lease_connector_v1_listener_handle_withdrawn(void *data,
			  struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
    printf("wp_drm_lease_connector_v1_listener_handle_withdrawn\n");

    struct state *state = data;
}

static const struct wp_drm_lease_connector_v1_listener wp_drm_lease_connector_v1_listener = {
    .name = wp_drm_lease_connector_v1_listener_handle_name,
    .description = wp_drm_lease_connector_v1_listener_handle_description,
    .connector_id = wp_drm_lease_connector_v1_listener_handle_connector_id,
    .done = wp_drm_lease_connector_v1_listener_handle_done,
    .withdrawn = wp_drm_lease_connector_v1_listener_handle_withdrawn,
};

// ----------------------------- drm_lease: device -----------------------------

static void wp_drm_lease_device_v1_listener_handle_drm_fd(void *data,
        struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1, int32_t fd)
{
    printf("wp_drm_lease_device_v1_listener_handle_drm_fd: %d\n", fd);

    struct state *state = data;
    state->drm_lease_device_fd = fd;

    printf("\tDevice path: %s\n", drmGetDeviceNameFromFd2(fd));
    printf("\tDevice is master? %s\n", drmIsMaster(fd) ? "Yes" : "No");
}

static void wp_drm_lease_device_v1_listener_handle_connector(void *data,
        struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1,
        struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
    printf("wp_drm_lease_device_v1_listener_handle_connector\n");

    struct state *state = data;
    wp_drm_lease_connector_v1_add_listener(wp_drm_lease_connector_v1,
            &wp_drm_lease_connector_v1_listener, state);
}

static void wp_drm_lease_device_v1_listener_handle_done(void *data,
        struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
    printf("wp_drm_lease_device_v1_listener_handle_done\n");
    struct state *state = data;
}

static void wp_drm_lease_device_v1_listener_handle_released(void *data,
        struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
    printf("wp_drm_lease_device_v1_listener_handle_released\n");
    struct state *state = data;
}

static const struct wp_drm_lease_device_v1_listener wp_drm_lease_device_v1_listener = {
    .drm_fd = wp_drm_lease_device_v1_listener_handle_drm_fd,
    .connector = wp_drm_lease_device_v1_listener_handle_connector,
    .done = wp_drm_lease_device_v1_listener_handle_done,
    .released = wp_drm_lease_device_v1_listener_handle_released,
};

// -------------------------------- wl_registry --------------------------------

static void registry_handle_global(void *data, struct wl_registry *wl_registry,
        uint32_t name, const char *interface, uint32_t version)
{
    struct state *state = data;

    printf("\tinterface = '%s', version = '%d', name = '%d'\n",
            interface, version, name);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(wl_registry, name,
                &wl_compositor_interface, 4);
                
    } else if (strcmp(interface, wp_drm_lease_device_v1_interface.name) == 0) {
        if (state->drm_lease_device) {
            printf("Multiple devices detected. To keep this client simple, only the first one is handled\n");
            return;
        }

        state->drm_lease_device = wl_registry_bind(wl_registry, name,
                &wp_drm_lease_device_v1_interface, 1);

        wp_drm_lease_device_v1_add_listener(state->drm_lease_device,
                &wp_drm_lease_device_v1_listener, state);
    }
}

static void registry_handle_global_remove(void *data,
        struct wl_registry *wl_registry, uint32_t name)
{
    // ---
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

// -----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    struct state state = { 0 };

    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        printf("Failed to connect to Wayland display.\n");
        return 1;
    }
    printf("Connection established!\n");

    printf("Checking global registry:\n");
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);

    if (!state.compositor) {
        printf("Error: wl_compositor not found\n");
        return 1;
    }

    if (!state.drm_lease_device) {
        printf("Error: wp_drm_lease_device_v1 not found\n");
        return 1;
    }

    while (wl_display_dispatch(display)) {
        thrd_sleep(&(struct timespec){.tv_sec=1}, NULL);
    }

    wl_display_disconnect(display);
    return 0;
}
