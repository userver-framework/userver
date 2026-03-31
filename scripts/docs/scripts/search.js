export function init_search_observer() {
  const searchResultsContainer = document.getElementById("SRResults");
  const observerOptions = {
    childList: true,
  };

  const highlightSearchResults = (records, observer) => {
    const searchValue = document
      .getElementById("MSearchField")
      .value.replace(/ +/g, " ");

    const searchResults = searchResultsContainer.querySelectorAll(
      '.SRResult[style="display: block;"]'
    );

    for (const searchResult of searchResults) {
      const searchResultSymbolNode = searchResult.querySelector(".SRSymbol");
      const searchResultValue = searchResultSymbolNode.textContent;

      const searchValueIndex = searchResultValue
        .trim()
        .toLowerCase()
        .indexOf(searchValue.trim().toLowerCase());

      searchResultSymbolNode.innerHTML =
        searchResultValue.slice(0, searchValueIndex) +
        '<span class="highlight">' +
        searchResultValue.slice(
          searchValueIndex,
          searchValueIndex + searchValue.length
        ) +
        "</span>" +
        searchResultValue.slice(searchValueIndex + searchValue.length);
    }
  };

  const observer = new MutationObserver(highlightSearchResults);
  observer.observe(searchResultsContainer, observerOptions);
};

export function init_all_results_button() {
  const searchBox = document.getElementById("MSearchResultsWindow");
  const searchInput = document.getElementById("MSearchField");

  const allResultsLink = document.createElement("a");
  allResultsLink.classList.add("all-results-link");

  allResultsLink.text = "All results";
  allResultsLink.target = "_blank";

  const searchURL = new URL("https://yandex.ru/search/");

  searchBox.appendChild(allResultsLink);

  searchInput.addEventListener("change", (event) => {
    searchURL.searchParams.set(
      "text",
      `site:userver.tech ${event.target.value}`
    );

    allResultsLink.href = searchURL;
  });
}

export function init_search_hotkey() {
  document.addEventListener("keydown", function(event) {
    if (event.key === "k" && event.ctrlKey) {
      event.preventDefault();
      document.getElementById("MSearchField").focus();
    }
  });
}
