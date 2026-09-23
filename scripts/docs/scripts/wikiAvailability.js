const TEAM_DOCS_AVAILABLE_COOKIE_NAME = "userver_team_docs_available";
const TEAM_DOCS_AVAILABLE_COOKIE_MAX_AGE_SECONDS = 365 * 24 * 60 * 60;
const SHOW_INTERNAL_LINKS_PARAM = "userver_show_internal_links";
const HIDE_INTERNAL_LINKS_PARAM_VALUE = "0";

const hasTeamDocsAvailableCookie = () => {
  return document.cookie.split(";").some((entry) => {
    const [name, value] = entry.trim().split("=");
    return name === TEAM_DOCS_AVAILABLE_COOKIE_NAME && value === "1";
  });
};

const setTeamDocsAvailableCookie = () => {
  document.cookie = `${TEAM_DOCS_AVAILABLE_COOKIE_NAME}=1; max-age=${TEAM_DOCS_AVAILABLE_COOKIE_MAX_AGE_SECONDS}; path=/; SameSite=Lax`;
};

const clearTeamDocsAvailableCookie = () => {
  document.cookie = `${TEAM_DOCS_AVAILABLE_COOKIE_NAME}=; max-age=0; path=/; SameSite=Lax`;
};

const getShowInternalLinksParamValue = () => {
  const params = new URLSearchParams(window.location.search);
  if (!params.has(SHOW_INTERNAL_LINKS_PARAM)) {
    return null;
  }

  return params.get(SHOW_INTERNAL_LINKS_PARAM);
};

export function evaluateInternalDocsMode() {
  const paramValue = getShowInternalLinksParamValue();

  if (paramValue === HIDE_INTERNAL_LINKS_PARAM_VALUE) {
    clearTeamDocsAvailableCookie();
    return false;
  }

  if (paramValue !== null) {
    setTeamDocsAvailableCookie();
    return true;
  }

  return hasTeamDocsAvailableCookie();
}

export function IsWikiAvailable() {
  return Promise.resolve(evaluateInternalDocsMode());
}
