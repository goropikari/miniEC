FROM qmkfm/base_container:latest

ARG VIAL_REVISION=dd43959ae5c08d8a28d38a1acf7b04e86b14a344

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc-avr avr-libc avrdude python3-venv \
    && rm -rf /var/lib/apt/lists/*

ENV QMK_HOME=/opt/vial-qmk
WORKDIR $QMK_HOME

RUN git init \
    && git fetch --depth 1 https://github.com/vial-kb/vial-qmk.git "$VIAL_REVISION" \
    && git checkout --detach FETCH_HEAD \
    && git remote add origin https://github.com/vial-kb/vial-qmk.git \
    && git submodule update --init --depth 1 lib/lufa \
    && python3 -m venv /opt/qmk \
    && /opt/qmk/bin/pip install --no-cache-dir qmk -r requirements.txt \
    && chmod 1777 "$QMK_HOME" \
    && git config --system --add safe.directory "$QMK_HOME"

ENV PATH="/opt/qmk/bin:${PATH}"
USER 1000:1000
