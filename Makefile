include Makefile.config

all: test

test: test-repo org.gtk.teleportapp.json
	flatpak-builder --force-clean --repo=test-repo --ccache --require-changes teleportapp org.gtk.teleportapp.json
	flatpak build-update-repo test-repo

release: repo org.gtk.teleportapp.json
	if [ "x${RELEASE_GPG_KEY}" == "x" ]; then echo Must set RELEASE_GPG_KEY in Makefile.config, try \'make gpg-key\'; exit 1; fi
	flatpak-builder --force-clean --repo=repo  --ccache --gpg-homedir=~/.gnupg --gpg-sign=${RELEASE_GPG_KEY} teleportapp  org.gtk.teleportapp.json
	flatpak build-update-repo --generate-static-deltas --gpg-homedir=~/.gnupg --gpg-sign=${RELEASE_GPG_KEY} repo

test-repo:
	ostree init --mode=archive-z2 --repo=test-repo

repo:
	ostree init --mode=archive-z2 --repo=repo

gpg-key:
	if [ "x${KEY_USER}" == "x" ]; then echo Must set KEY_USER in Makefile.config; exit 1; fi
	mkdir -p gpg
	gpg2 --homedir gpg --quick-gen-key ${KEY_USER}
	echo Enter the above gpg key id as RELEASE_GPG_KEY in Makefile.config

teleportapp.flatpakref: teleportapp.flatpakref.in
	sed -e 's|@URL@|${URL}|g' -e 's|@GPG@|$(shell gpg2 --homedir=~/.gnupg --export ${RELEASE_GPG_KEY} | base64 | tr -d '\n')|' $< > $@
