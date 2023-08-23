window.addEventListener("load", () => {
  const searchResultsContainer = document.getElementById("SRResults");
  const observerOptions = {
    childList: true,
  };

  const highlightSearchResults = (records, observer) => {
    const searchValue = document
      .getElementById("MSearchField")
      .value.replace(/ +/g, "");

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
});
