# eCTF Exemplars
This repository holds exemplar implementations of the 2025 Embedded Capture
the Flag (eCTF) Competition.

Information about the eCTF can be found at https://ectf.mitre.org and more
information aobut the 2025 eCTF competition can be found at
https://rules.ectf.mitre.org/2025.

## Setup
Requires:
* Python >=3.12


## Layout
There are two main directories of interest. First is `tools/`, which holds
a Python package that automates interaction with the various desings. This
will be installed as `ectf25` in the created virtual environments.

Second is `src`, which holds the source code to four implementations of the
2025 eCTF challenge. The first is `src/insecure`, which contains the reference
design that implements only the functional satellite TV system with no
intention of addressing the security requirements. The other three are
`src/design1`, `src/design2`, and `src/design3`, which contain three full
attempts at completing the competition, each demonstrating more success than
the last.

## Use
### 1. Clone Repository
First, you must clone the repository.

If you have git, you can use it directly, otherwise, you can use Docker to clone it for you:

```powershell
# with git
git clone https://github.com/ectfmitre/ectf-workshop.git C:\Users\Student\Desktop\workshop
```

### 2. Install the tools

```powershell

python -m pip install .\tools

```

### 3. Download challenge zip from scoreboard
Pick a challenge and download its zip file

### 4. Flash Firmware
We can now flash the firmware onto the boards with:

```powershell
python -m ectf25.utils.flash <encrypted_binary> $serial_dev
```

Where the `encrypted_binary` is the "attacker.prot" file found in the challenge zip

NOTE: The board must be in update mode to flash. When in update mode, there
should be a blue LED that blinks on and off roughly once per second. You can
put the board in update mode by holding the button SW1 on the board while you
plug it in to USB.

### 5. Interact with Decoder
With the decoder flashed, you can now interract with it.

The three commands available are:

#### 5.1 List
You can list all subscribed channels with:

```powershell
python -m ectf25.tv.list $serial_dev
```

#### 5.2 Subscribe
You can subscribe the decoder to a new channel or update the existing channel
subscription with:

```powershell
python -m ectf25.tv.subscribe .\always_1.sub $serial_dev
```

Where `always_1.sub` is the generated subscription.

#### 5.3 Run
Finally, you can run the TV with:

```powershell
python -m ectf25.tv.run <sat_host_ip> <sat_challenge_port> $serial
```

This will stream the encoded payloads to the board for a given challenge. YOu can find the correct port number in the challenge description. 
