FROM qmkfm/base_container:latest

ARG VIAL_REVISION=dd43959ae5c08d8a28d38a1acf7b04e86b14a344

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc-avr avr-libc avrdude python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --shell /bin/bash vscode \
    && git init /home/vscode/vial-qmk \
    && git -C /home/vscode/vial-qmk fetch --depth 1 https://github.com/vial-kb/vial-qmk.git "$VIAL_REVISION" \
    && git -C /home/vscode/vial-qmk checkout --detach FETCH_HEAD \
    && git -C /home/vscode/vial-qmk remote add origin https://github.com/vial-kb/vial-qmk.git \
    && git -C /home/vscode/vial-qmk submodule update --init --depth 1 lib/lufa \
    && python3 -m venv /opt/qmk \
    && /opt/qmk/bin/pip install --no-cache-dir qmk -r /home/vscode/vial-qmk/requirements.txt \
    && chown -R vscode:vscode /home/vscode/vial-qmk \
    && chmod 1777 /home/vscode/vial-qmk \
    && git config --system --add safe.directory /home/vscode/vial-qmk

ENV PATH="/opt/qmk/bin:${PATH}"
ENV QMK_HOME=/home/vscode/vial-qmk
WORKDIR /home/vscode/vial-qmk
USER vscode
